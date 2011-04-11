from skywriting.runtime.executors import ProcessRunningExecutor
from skywriting.runtime.exceptions import BlameUserException
    
import logging
import subprocess
import os, stat
import cherrypy

class _CloudProcessCommonExecutor(ProcessRunningExecutor):
    def __init__(self, worker):
        self.env = {}
        ProcessRunningExecutor.__init__(self, worker)
        
    
    def start_process(self, input_files, output_files):
        
        try:
            self.env.update( env = self.args['env'] )
        except KeyError:
            pass

        for i, ref in enumerate(self.input_refs):
            self.env['CL_PATH_' + ref.id] = input_files[i]
        
        self.env['CL_TASK_ID'] = self.task_id
        self.env['CL_OUTPUT_ID'] = self.output_ids[0]
        self.env['CL_OUTPUT_FILE'] = output_files[0]
        self.env['CL_JOB_ID'] = self.task_record.task_descriptor['job']
        
        self.env['CL_BLOCK_STORE'] = self.task_record.block_store.base_dir
        self.env['CL_WORKER_LOC'] = self.task_record.block_store.netloc.replace("http://", "", 1)
        self.env['CL_MASTER_LOC'] = self.task_record.master_proxy.master_url.replace("http://", "", 1)
        
        self.before_execute()
        proc = subprocess.Popen( map(str, self.get_process_args()), shell=False, close_fds=True, env=self.env )
        self.process_manage(proc)
        
        return proc
    
    def process_manage(self, proc):
        return

    def get_process_args(self):
        return [];
        
    def _abort(self):
        if self.proc is not None:
            self.proc.kill()

class CloudAppExecutor(_CloudProcessCommonExecutor):
    
    handler_name = "cloudapp"
    
    def __init__(self, worker):
        _CloudProcessCommonExecutor.__init__(self, worker)
        
    def before_execute(self):
        try:
            self.app_ref = self.args['app_ref']
            self.app_args = self.args.get('app_args', [])
        except KeyError:
            raise BlameUserException('Incorrect arguments to the CloudApp: %s' % repr(self.args))
        
        self.filename = self.block_store.retrieve_filename_for_ref(self.app_ref)
        os.chmod(self.filename, stat.S_IRWXU)
        
    def get_process_args(self):
        cherrypy.log.error("package path: %s" % self.filename, "CloudAppExecutor", logging.INFO)
        return [self.filename] + self.app_args
        
        
class CloudThreadExecutor(_CloudProcessCommonExecutor):
    
    handler_name = "cldthread"
    
    def __init__(self, worker):
        _CloudProcessCommonExecutor.__init__(self, worker)


    def before_execute(self):
        
        try:
            self.checkpoint_ref = self.args['checkpoint']
        except KeyError:
            raise BlameUserException('Incorrect arguments to the CloudThread executor: %s' % repr(self.args))
        
        self.filename = self.block_store.retrieve_filename_for_ref(self.checkpoint_ref)
        
        self.sync_fifo_path = os.path.join('/tmp/', self.task_id + ":sync")
        self.comm_fifo_path = os.path.join('/tmp/', self.task_id + ":comm")
        
        os.mkfifo(self.sync_fifo_path)
        os.mkfifo(self.comm_fifo_path)
        
    def process_manage(self, proc):
        
        cherrypy.log.error("Opening named pipe: %s" % self.comm_fifo_path, "CloudThreadExecutor", logging.INFO)
        
        # This blocks until the C process opens up the pipe for reading
        fifo = open(self.comm_fifo_path, 'w')
        
        #cherrypy.log.error("Writing to named pipe: %s" % self.comm_fifo_path, "CloudThreadExecutor", logging.INFO)
        
        for name, value in self.env.items():
            fifo.write("%s\n%s\n" % (name, value) );
        
        #cherrypy.log.error("Closing named pipe: %s" % self.comm_fifo_path, "CloudThreadExecutor", logging.INFO)
        
        fifo.close()
        
        cherrypy.log.error("Closed named pipe: %s" % self.comm_fifo_path, "CloudThreadExecutor", logging.INFO)
        
        #Sync to make sure that the FDs are ready
        open(self.sync_fifo_path, 'r').close()
        os.remove(self.sync_fifo_path)
        

    def get_process_args(self):
        cherrypy.log.error("checkpoint path : %s" % self.filename, "CloudThreadExecutor", logging.INFO)
        return ["cr_restart", "--no-restore-pid", self.filename ]

