# Copyright (c) 2010 Derek Murray <derek.murray@cl.cam.ac.uk>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
from skywriting.runtime.references import SW2_ConcreteReference,\
    SW2_TombstoneReference
from cherrypy.process import plugins
import urllib2
from skywriting.runtime.block_store import BLOCK_LIST_RECORD_STRUCT,\
    json_decode_object_hook
import logging
import cherrypy
import os
import simplejson
from skywriting.runtime.master.job_pool import RECORD_HEADER_STRUCT,\
    Job, JOB_ACTIVE
from skywriting.runtime.task import build_taskpool_task_from_descriptor
import httplib2

class TaskFailureInvestigator:
    
    def __init__(self, task_pool, worker_pool, deferred_worker):
        self.task_pool = task_pool
        self.worker_pool = worker_pool
        self.deferred_worker = deferred_worker
        
    def investigate_task_failure(self, task_id, failure_payload):
        self.deferred_worker.do_deferred(lambda: self._investigate_task_failure(task_id, failure_payload))
        
    def _investigate_task_failure(self, task, failure_payload):
        (reason, detail, bindings) = failure_payload
        cherrypy.log.error('Investigating failure of task %s' % task.task_id, 'TASKFAIL', logging.WARN)
        cherrypy.log.error('Task failed because %s' % reason, 'TASKPOOL', logging.WARN)

        revised_bindings = {}
        failed_netlocs = set()

        # First, go through the bindings to determine which references are really missing.
        for id, tombstone in bindings.items():
            if isinstance(tombstone, SW2_TombstoneReference):
                cherrypy.log.error('Investigating reference: %s' % str(tombstone), 'TASKFAIL', logging.WARN)
                failed_netlocs_for_ref = set()
                for netloc in tombstone.netlocs:
                    h = httplib2.Http()
                    try:
                        response, _ = h.request('http://%s/data/%s' % (netloc, id), 'HEAD')
                        if response['status'] != '200':
                            cherrypy.log.error('Could not obtain object from %s: status %s' % (netloc, response['status']), 'TASKFAIL', logging.WARN)
                            failed_netlocs_for_ref.add(netloc)
                        else:
                            cherrypy.log.error('Object still available from %s' % netloc, 'TASKFAIL', logging.INFO)
                    except:
                        cherrypy.log.error('Could not contact store at %s' % netloc, 'TASKFAIL', logging.WARN)
                        failed_netlocs.add(netloc)
                        failed_netlocs_for_ref.add(netloc)
                if len(failed_netlocs_for_ref) > 0:
                    revised_bindings[id] = SW2_TombstoneReference(id, failed_netlocs_for_ref)
            else:
                # Could potentially mention a ConcreteReference or something similar in the failure bindings.
                revised_bindings[id] = tombstone
                
        # Now, having collected a set of actual failures, deem those workers to have failed.
        for netloc in failed_netlocs:
            worker = self.worker_pool.get_worker_at_netloc(netloc)
            if worker is not None:
                self.worker_pool.worker_failed(worker)
                
        # Finally, propagate the failure to the task pool, so that we can re-run the failed task.
        self.task_pool.task_failed(task, (reason, detail, revised_bindings))

class RecoveryManager(plugins.SimplePlugin):
    
    def __init__(self, bus, job_pool, task_pool, block_store, deferred_worker):
        plugins.SimplePlugin.__init__(self, bus)
        self.job_pool = job_pool
        self.task_pool = task_pool
        self.block_store = block_store
        self.deferred_worker = deferred_worker
        
    def subscribe(self):
        # In order to present a consistent view to clients, we must do these
        # before starting the webserver.
        self.bus.subscribe('start', self.recover_local_blocks, 5)
        self.bus.subscribe('start', self.recover_job_descriptors, 10)
        
        
        self.bus.subscribe('fetch_block_list', self.fetch_block_list_defer)
    
    def unsubscribe(self):
        self.bus.unsubscribe('start', self.recover_local_blocks)
        self.bus.unsubscribe('start', self.recover_job_descriptors)
        self.bus.unsubscribe('fetch_block_list', self.fetch_block_list_defer)

    def recover_local_blocks(self):
        if not self.block_store.is_empty():
            for block_name, block_size in self.block_store.block_list_generator():
                conc_ref = SW2_ConcreteReference(block_name, block_size)
                conc_ref.add_location_hint(self.block_store.netloc)
                #cherrypy.log.error('Recovering block %s (size=%d)' % (block_name, block_size), 'RECOVERY', logging.INFO)
                self.task_pool.publish_single_ref(block_name, conc_ref, None, False)                

    def recover_job_descriptors(self):
        root = self.job_pool.journal_root
        if root is None:
            return
        
        for job_id in os.listdir(root):

            try:
                job_dir = os.path.join(root, job_id)
                result_path = os.path.join(job_dir, 'result')
                if os.path.exists(result_path):
                    with open(result_path, 'r') as result_file:
                        result = simplejson.load(result_file, object_hook=json_decode_object_hook)
                else:
                    result = None
                    
                journal_path = os.path.join(job_dir, 'task_journal')
                journal_file = open(journal_path, 'rb')
                record_type, root_task_descriptor_length = RECORD_HEADER_STRUCT.unpack(journal_file.read(RECORD_HEADER_STRUCT.size))
                root_task_descriptor_string = journal_file.read(root_task_descriptor_length)
                assert record_type == 'T'
                assert len(root_task_descriptor_string) == root_task_descriptor_length
                root_task_descriptor = simplejson.loads(root_task_descriptor_string, object_hook=json_decode_object_hook)
                root_task_id = root_task_descriptor['task_id']
                root_task = build_taskpool_task_from_descriptor(root_task_id, root_task_descriptor, self.task_pool, None)
                job = Job(job_id, root_task, job_dir)
                root_task.job = job
                if result is not None:
                    job.completed(result)
                self.job_pool.add_job(job)
                self.task_pool.add_task(root_task)
                
                if result is None:
                    self.load_other_tasks_defer(job, journal_file)
                    cherrypy.log.error('Recovered job %s' % job_id, 'RECOVERY', logging.INFO, False)
                    cherrypy.log.error('Recovered task %s for job %s' % (root_task_id, job_id), 'RECOVERY', logging.INFO, False)
                else:
                    journal_file.close()
                    cherrypy.log.error('Found information about job %s' % job_id, 'RECOVERY', logging.INFO, False)
                
                
            except:
                # We have lost critical data for the job, so we must fail it.
                cherrypy.log.error('Error recovering job %s' % job_id, 'RECOVERY', logging.ERROR, True)
                self.job_pool.add_failed_job(job_id)

    def load_other_tasks_defer(self, job, journal_file):
        self.deferred_worker.do_deferred(lambda: self.load_other_tasks_for_job(job, journal_file))

    def load_other_tasks_for_job(self, job, journal_file):
        '''
        Process a the task journal for a recovered job.
        '''
        try:
            while True:
                record_header = journal_file.read(RECORD_HEADER_STRUCT.size)
                if len(record_header) != RECORD_HEADER_STRUCT.size:
                    cherrypy.log.error('Journal entry truncated for job %s' % job.id, 'RECOVERY', logging.WARNING, False)
                    break
                record_type, record_length = RECORD_HEADER_STRUCT.unpack(record_header)
                record_string = journal_file.read(record_length)
                if len(record_string) != record_length:
                    cherrypy.log.error('Journal entry truncated for job %s' % job.id, 'RECOVERY', logging.WARNING, False)
                    break
                rec = simplejson.loads(record_string, object_hook=json_decode_object_hook)
                if record_type == 'R':
                    self.task_pool.publish_single_ref(rec['id'], rec['ref'], job, should_journal=False)
                elif record_type == 'T':
                    task_id = rec['task_id']
                    parent_task = self.task_pool.get_task_by_id(rec['parent'])
                    task = build_taskpool_task_from_descriptor(task_id, rec, self.task_pool, parent_task)
                    task.job = job
                    task.parent.children.append(task)
    
                    cherrypy.log.error('Recovered task %s for job %s' % (task_id, job.id), 'RECOVERY', logging.INFO, False)
                    self.task_pool.add_task(task)
                else:
                    cherrypy.log.error('Got invalid record type in job %s' % job.id, 'RECOVERY', logging.WARNING, False)
                
        except:
            cherrypy.log.error('Error recovering task_journal for job %s' % job.id, 'RECOVERY', logging.WARNING, True)

        finally:
            journal_file.close()
            if job.state == JOB_ACTIVE:
                cherrypy.log.error('Restarting recovered job %s' % job.id, 'RECOVERY', logging.INFO)
            self.job_pool.restart_job(job)

    def fetch_block_list_defer(self, worker):
        self.deferred_worker.do_deferred(lambda: self.fetch_block_names_from_worker(worker))
        
    def fetch_block_names_from_worker(self, worker):
        '''
        Loop through the block list file from the given worker, and publish the
        references found there.
        '''
        
        block_file = urllib2.urlopen('http://%s/data/' % worker.netloc)

        while True:
            record = block_file.read(BLOCK_LIST_RECORD_STRUCT.size)
            if not record:
                break
            block_name, block_size = BLOCK_LIST_RECORD_STRUCT.unpack(record)
            conc_ref = SW2_ConcreteReference(block_name, block_size)
            conc_ref.add_location_hint(worker.netloc)
            
            #cherrypy.log.error('Recovering block %s (size=%d)' % (block_name, block_size), 'RECOVERY', logging.INFO)
            self.task_pool.publish_single_ref(block_name, conc_ref, None, False)

        
        block_file.close()
        
        # Publishing recovered blocks may cause tasks to become QUEUED, so we
        # must run the scheduler.
        self.bus.publish('schedule')
        