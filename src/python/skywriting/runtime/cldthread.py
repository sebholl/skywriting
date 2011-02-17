from skywriting.runtime.executors import \
    SWExecutor, add_running_child, remove_running_child
from skywriting.runtime.references import \
    SW2_ConcreteReference
from skywriting.runtime.exceptions import \
    BlameUserException, MissingInputException
    
import logging
import subprocess
import os
import cherrypy
from threading import Lock

class CloudThreadTaskExecutionRecord:
    
    def __init__(self, task_descriptor, task_executor):
        self.task_id = task_descriptor['task_id']
        self.task_executor = task_executor
        self.inputs = task_descriptor['inputs']
        self.expected_outputs = task_descriptor['expected_outputs'] 
        self.executor = None
        self.is_running = True
        self._lock = Lock()
    
    def abort(self):
        self.is_running = False
        with self._lock:
            # Guards against missing abort because the executor is started between test and call
            if self.executor is not None:
                self.executor.abort()

    def notify_streams_done(self):
        # Out-of-thread call
        with self._lock:
            if self.executor is not None:
                self.executor.notify_streams_done()
    
    def commit(self):
        commit_bindings = {}
        for i, output_ref in enumerate(self.executor.output_refs):
            commit_bindings[self.expected_outputs[i]] = output_ref
        self.task_executor.master_proxy.commit_task(self.task_id, commit_bindings)
    
    def fetch_executor_args(self):
        args_ref = None
        parsed_inputs = {}
        
        for local_id, ref in self.inputs.iteritems():
            if local_id == '_args':
                args_ref = ref
            else:
                parsed_inputs[local_id] = ref
        
        self.task_executor.block_store.retrieve_filenames_for_refs_eager( parsed_inputs.values() )
        
        return self.task_executor.block_store.retrieve_object_for_ref(args_ref, 'json')
    
    def execute(self):        
        try:
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Choosing appropriate CloudThread executor")
                executor = CloudThreadExecutor( self.fetch_executor_args(), None, self.expected_outputs, self.task_executor.master_proxy)
                with self._lock:
                    self.executor = executor
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Executing")
                self.executor.execute(self.task_executor.block_store, self.task_id)
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Committing")
                self.commit()
            else:
                self.task_executor.master_proxy.failed_task(self.task_id)
        except MissingInputException as mie:
            cherrypy.log.error('Missing input during CloudThread task execution', 'CloudThread', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, 'MISSING_INPUT', bindings=mie.bindings)
        except Exception as e:
            cherrypy.log.error('Error during executor task execution', 'CloudThread', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, str(e))

class _CloudProcessCommonExecutor(SWExecutor):

    def __init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit=None):
        SWExecutor.__init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit)
        
        self.args = args
        self.proc = None
        self.env = {}

        self.env['SW_MASTER_LOC'] = master_proxy.master_url.replace("http://", "", 1)
        self.env['SW_OUTPUT_ID'] = expected_output_ids[0]
    
    def start_process(self, block_store):

        self.before_execute(block_store)
        cherrypy.engine.publish("worker_event", "Executor: running")
        
        env = os.environ.copy();
        env.update(self.env);
        
        proc = subprocess.Popen(self.get_process_args(), shell=False, close_fds=True, env=env )
        self.process_manage(proc)
        
        return proc
    
    def process_manage(self, proc):
        return
    
    def before_execute(self, block_store):
        return

    def get_process_args(self):
        return [];

    def _execute(self, block_store, task_id):
        
        self.task_id = task_id
        
        self.env['SW_WORKER_LOC'] = block_store.netloc.replace("http://", "", 1)
        self.env['SW_BLOCK_STORE'] = block_store.base_dir
        self.env['SW_TASK_ID'] = task_id
        
        self.proc = self.start_process(block_store)
        add_running_child(self.proc)

        rc = self.proc.wait()
        remove_running_child(self.proc)

        self.proc = None

        cherrypy.engine.publish("worker_event", "Executor: Waiting for transfers (for cache)")

        if rc != 20:
            
            if rc == 0:
                cherrypy.engine.publish("worker_event", "Executor: PackagedApp exited successfully")
                # XXX: fix size_hint and related.
                # block_store.store_object(True, 'json', self.output_ids[0])
                
                real_ref = SW2_ConcreteReference(self.output_ids[0], 0)
                real_ref.add_location_hint(block_store.netloc)
                self.output_refs[0] = real_ref
            else:
                cherrypy.log.error( "Process terminated with unexpected return code (%s)." % rc, "EXEC", logging.ERROR )
                raise OSError()
            
        else:
            cherrypy.engine.publish("worker_event", "Executor: PackagedApp expecting to be resumed")
        
    def _abort(self):
        if self.proc is not None:
            self.proc.kill()

class CloudAppExecutor(_CloudProcessCommonExecutor):
    def __init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit=None):
        _CloudProcessCommonExecutor.__init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit)
        
        try:
            self.app_ref = args['app_ref']
            self.app_args = args.get('app_args', [])
        except KeyError:
            raise BlameUserException('Incorrect arguments to the CloudApp: %s' % repr(args))
        
        
    def get_process_args(self):
        cherrypy.log.error("CloudAppExecutor package path : %s" % self.filenames, "CloudAppExecutor", logging.INFO)
        return self.filenames + self.app_args
    
    def before_execute(self, block_store):
        cherrypy.log.error("Running CloudAppExecutor for : %s" % self.app_ref, "CloudAppExecutor", logging.INFO)
        self.filenames = self.get_filenames_eager(block_store, [self.app_ref])
        
        
class CloudThreadExecutor(_CloudProcessCommonExecutor):

    def __init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit=None):
        _CloudProcessCommonExecutor.__init__(self, args, continuation, expected_output_ids, master_proxy, fetch_limit)
        try:
            self.checkpoint_ref = self.args['checkpoint']
        except KeyError:
            raise BlameUserException('Incorrect arguments to the CloudThread executor: %s' % repr(self.args))

    def before_execute(self, block_store):
        cherrypy.log.error("Running CloudThread executor for checkpoint: %s" % self.checkpoint_ref, "CloudThreadExecutor", logging.INFO)
        self.checkpoint_filenames = self.get_filenames_eager(block_store, [self.checkpoint_ref])
        
    def process_manage(self, proc):
        filename = os.path.join('/tmp/', self.task_id)
        
        cherrypy.log.error("Opening named pipe: %s" % filename, "CloudThreadExecutor", logging.INFO)
        
        os.mkfifo(filename)
        fifo = open(filename, 'w')
        
#        cherrypy.log.error("Writing to named pipe: %s" % filename, "CloudThreadExecutor", logging.INFO)
        
        for name, value in self.env.items():
            fifo.write("%s\n%s\n" % (name, value) );
        
#        cherrypy.log.error("Closing named pipe: %s" % filename, "CloudThreadExecutor", logging.INFO)
        
        fifo.close()
        
        cherrypy.log.error("Closed named pipe: %s" % filename, "CloudThreadExecutor", logging.INFO)

    def get_process_args(self):
        cherrypy.log.error("CloudThreadExecutor checkpoint path : %s" % self.checkpoint_filenames, "CloudThreadExecutor", logging.INFO)
        return ["cr_restart", "--no-restore-pid", self.checkpoint_filenames[0] ]

