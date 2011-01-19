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
from __future__ import with_statement
from skywriting.lang.parser import CloudScriptParser
import urlparse
from skywriting.runtime.plugins import AsynchronousExecutePlugin
from skywriting.lang.context import SimpleContext, TaskContext,\
    LambdaFunction
from skywriting.lang.datatypes import all_leaf_values, map_leaf_values
from skywriting.lang.visitors import \
    StatementExecutorVisitor, SWDereferenceWrapper
from skywriting.lang import ast
from skywriting.runtime.exceptions import ReferenceUnavailableException,\
    FeatureUnavailableException, ExecutionInterruption,\
    SelectException, MissingInputException, MasterNotRespondingException,\
    BlameUserException
from threading import Lock
import cherrypy
import logging
import uuid
import hashlib
from skywriting.runtime.references import SWDataValue, SWURLReference,\
    SWRealReference,\
    SWErrorReference, SW2_FutureReference,\
    SW2_ConcreteReference

class TaskExecutorPlugin(AsynchronousExecutePlugin):
    
    def __init__(self, bus, block_store, master_proxy, execution_features, execution_task_record_types, num_threads=1):
        AsynchronousExecutePlugin.__init__(self, bus, num_threads, "execute_task")
        self.block_store = block_store
        self.master_proxy = master_proxy
        self.execution_features = execution_features
        self.execution_task_record_types = execution_task_record_types
        self.current_task_id = None
        self.current_task_execution_record = None
    
        self._lock = Lock()
    
    def abort_task(self, task_id):
        with self._lock:
            if self.current_task_id == task_id:
                self.current_task_execution_record.abort()
            self.current_task_id = None
            self.current_task_execution_record = None

    def notify_streams_done(self, task_id):
        with self._lock:
            # Guards against changes to self.current_{task_id, task_execution_record}
            if self.current_task_id == task_id:
                # Note on threading: much like aborts, the execution_record's execute() is running
                # in another thread. It might have not yet begun, already completed, or be in progress.
                self.current_task_execution_record.notify_streams_done()
    
    def handle_input(self, input):
        handler = input['handler']

        execution_record = self.execution_task_record_types.get(handler, SWExecutorTaskExecutionRecord )(input, self)

        with self._lock:
            self.current_task_id = input['task_id']
            self.current_task_execution_record = execution_record
        
        cherrypy.engine.publish("worker_event", "Start execution " + repr(input['task_id']) + " with handler " + input['handler'])
        cherrypy.log.error("Starting task %s with handler %s" % (str(self.current_task_id), handler), 'TASK', logging.INFO, False)
        try:
            execution_record.execute()
            cherrypy.engine.publish("worker_event", "Completed execution " + repr(input['task_id']))
            cherrypy.log.error("Completed task %s with handler %s" % (str(self.current_task_id), handler), 'TASK', logging.INFO, False)
        except:
            cherrypy.log.error("Error in task %s with handler %s" % (str(self.current_task_id), handler), 'TASK', logging.ERROR, True)

        with self._lock:
            self.current_task_id = None
            self.current_task_execution_record = None
            
            
class ReferenceTableEntry:
    
    def __init__(self, reference):
        self.reference = reference
        self.is_dereferenced = False
        self.is_execd = False
        self.is_returned = False
        
    def __repr__(self):
        return 'ReferenceTableEntry(%s, d=%s, e=%s, r=%s)' % (repr(self.reference), repr(self.is_dereferenced), repr(self.is_execd), repr(self.is_returned))
        
class SpawnListEntry:
    
    def __init__(self, id, task_descriptor, continuation=None):
        self.id = id
        self.task_descriptor = task_descriptor
        self.continuation = continuation
        self.ignore = False
    
class SWContinuation:
    
    def __init__(self, task_stmt, context):
        self.task_stmt = task_stmt
        self.current_local_id_index = 0
        self.stack = []
        self.context = context
        self.reference_table = {}
      
    def __repr__(self):
        return "SWContinuation(task_stmt=%s, current_local_id_index=%s, stack=%s, context=%s, reference_table=%s)" % (repr(self.task_stmt), repr(self.current_local_id_index), repr(self.stack), repr(self.context), repr(self.reference_table))

    def create_tasklocal_reference(self, ref):
        id = self.current_local_id_index
        self.current_local_id_index += 1
        self.reference_table[id] = ReferenceTableEntry(ref)
        return SWLocalReference(id)

    def store_tasklocal_reference(self, id, ref):
        """Used when copying references to a spawned continuation."""
        self.reference_table[id] = ReferenceTableEntry(ref)
        self.current_local_id_index = max(self.current_local_id_index, id + 1)
    
    # The following methods capture why we might have blocked on something,
    # for appropriate handling on task loading.
    def mark_as_dereferenced(self, ref):
        self.reference_table[ref.index].is_dereferenced = True
    def is_marked_as_dereferenced(self, id):
        return self.reference_table[id].is_dereferenced
    def mark_as_execd(self, ref):
        self.reference_table[ref.index].is_execd = True
    def is_marked_as_execd(self, id):
        return self.reference_table[id].is_execd
    def mark_as_returned(self, ref):
        self.reference_table[ref.index].is_returned = True
    def is_marked_as_returned(self, id):
        return self.reference_table[id].is_returned
        
    def rewrite_reference(self, id, real_ref):
        self.reference_table[id].reference = real_ref
        
    def resolve_tasklocal_reference_with_index(self, index):
        return self.reference_table[index].reference
    def resolve_tasklocal_reference_with_ref(self, ref):
        return self.reference_table[ref.index].reference

class SafeLambdaFunction(LambdaFunction):
    
    def __init__(self, function, interpreter):
        LambdaFunction.__init__(self, function)
        self.interpreter = interpreter

    def call(self, args_list, stack, stack_base, context):
        safe_args = self.interpreter.do_eager_thunks(args_list)
        return LambdaFunction.call(self, safe_args, stack, stack_base, context)

class SWLocalReference:
    """
    A primitive object used in the interpreter, and returned from functions like
    ref() and spawn(). Contains an index into the continuation's reference table,
    which identifies the real reference object.
    """
    
    def __init__(self, index):
        self.index = index
        
    def as_tuple(self):
        return ('local', self.index)

    def __repr__(self):
        return 'SWLocalReference(%d)' % self.index

class SWExecutorTaskExecutionRecord:
    
    def __init__(self, task_descriptor, task_executor):
        self.task_id = task_descriptor['task_id']
        try:
            self.original_task_id = task_descriptor['original_task_id']
        except KeyError:
            self.original_task_id = self.task_id
        self.expected_outputs = task_descriptor['expected_outputs'] 
        self.task_executor = task_executor
        self.inputs = task_descriptor['inputs']
        self.executor_name = task_descriptor['handler']
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
    
    def fetch_executor_args(self, inputs):
        args_ref = None
        parsed_inputs = {}
        
        for local_id, ref in inputs.items():
            if local_id == '_args':
                args_ref = ref
            else:
                parsed_inputs[int(local_id)] = ref
        
        exec_args = self.task_executor.block_store.retrieve_object_for_ref(args_ref, 'pickle')
        
        def args_parsing_mapper(leaf):
            if isinstance(leaf, SWLocalReference):
                return parsed_inputs[leaf.index]
            else:
                return leaf
        
        parsed_args = map_leaf_values(args_parsing_mapper, exec_args)
        return parsed_args
    
    def commit(self):
        commit_bindings = {}
        for i, output_ref in enumerate(self.executor.output_refs):
            commit_bindings[self.expected_outputs[i]] = output_ref
        self.task_executor.master_proxy.commit_task(self.task_id, commit_bindings)
    
    def execute(self):        
        try:
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Fetching args")
                parsed_args = self.fetch_executor_args(self.inputs)
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Fetching executor")
                executor = self.task_executor.execution_features.get_executor(self.executor_name, parsed_args, None, self.expected_outputs, self.task_executor.master_proxy)
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
            cherrypy.log.error('Missing input during SWI task execution', 'SWI', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, 'MISSING_INPUT', bindings=mie.bindings)
        except:
            cherrypy.log.error('Error during executor task execution', 'EXEC', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, 'RUNTIME_EXCEPTION')
            
class SWInterpreterTaskExecutionRecord:
    
    def __init__(self, task_descriptor, task_executor):
        self.task_id = task_descriptor['task_id']
        self.task_executor = task_executor
        
        self.is_running = True
        self.is_fetching = False
        self.is_interpreting = False
        
        try:
            self.interpreter = SWRuntimeInterpreterTask(task_descriptor, self.task_executor.block_store, self.task_executor.execution_features, self.task_executor.master_proxy)
        except:
            cherrypy.log.error('Error during SWI task creation', 'SWI', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id)            

    def execute(self):
        try:
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Fetching args")
                self.interpreter.fetch_inputs(self.task_executor.block_store)
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Interpreting")
                self.interpreter.interpret()
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Spawning")
                self.interpreter.spawn_all(self.task_executor.block_store, self.task_executor.master_proxy)
            if self.is_running:
                cherrypy.engine.publish("worker_event", "Committing")
                self.interpreter.commit_result(self.task_executor.block_store, self.task_executor.master_proxy)
            else:
                self.task_executor.master_proxy.failed_task(self.task_id)
        
        except MissingInputException as mie:
            cherrypy.log.error('Missing input during SWI task execution', 'SWI', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, 'MISSING_INPUT', bindings=mie.bindings)
              
        except MasterNotRespondingException:
            cherrypy.log.error('Could not commit task results to the master', 'SWI', logging.ERROR, True)
                
        except:
            cherrypy.log.error('Error during SWI task execution', 'SWI', logging.ERROR, True)
            self.task_executor.master_proxy.failed_task(self.task_id, 'RUNTIME_EXCEPTION')    

    def abort(self):
        self.is_running = False
        self.interpreter.abort()
        
#class SWBLCRTaskExecutionRecord( SWExecutorTaskExecutionRecord ):
#    
#    def __init__(self, task_descriptor, task_executor):
#        self.task_id = task_descriptor['task_id']
#        self.task_executor = task_executor
#        
#        self.is_running = True
#        self.is_fetching = False
#        self.is_interpreting = False
#        
#    def execute(self):
#        try:
#            if self.is_running:
#                cherrypy.engine.publish("worker_event", "Fetching checkpoint")
#                self.interpreter.fetch_inputs(self.task_executor.block_store)
#            if self.is_running:
#                cherrypy.engine.publish("worker_event", "Starting checkpoint")
#                self.interpreter.interpret()
#            if self.is_running:
#                cherrypy.engine.publish("worker_event", "Finished - (we should probs commit at some point)")
#            else:
#                self.task_executor.master_proxy.failed_task(self.task_id)
#         
#        except:
#            cherrypy.log.error('Error during BLCR task execution', 'BLCR', logging.ERROR, True)
#            self.task_executor.master_proxy.failed_task(self.task_id, 'RUNTIME_EXCEPTION')    
#
#    def abort(self):
#        self.is_running = False
#        self.interpreter.abort()
        
class SWRuntimeInterpreterTask:
    
    def __init__(self, task_descriptor, block_store, execution_features, master_proxy): # scheduler, task_expr, is_root=False, result_ref_id=None, result_ref_id_list=None, context=None, condvar=None):
        self.task_id = task_descriptor['task_id']
        
        try:
            self.original_task_id = task_descriptor['original_task_id']
            self.replay_ref = task_descriptor['replay_ref']
        except KeyError:
            self.original_task_id = self.task_id
            self.replay_ref = None

        self.additional_refs_to_publish = []
        
        self.expected_outputs = task_descriptor['expected_outputs']
        self.inputs = task_descriptor['inputs']

        try:
            self.select_result = task_descriptor['select_result']
        except KeyError:
            self.select_result = None
            
        try:
            self.save_continuation = task_descriptor['save_continuation']
        except KeyError:
            self.save_continuation = False
            
        try:
            self.replay_uuid_list = task_descriptor['replay_uuids']
            self.replay_uuids = True
            self.current_uuid_index = 0
        except KeyError:
            self.replay_uuid_list = []
            self.replay_uuids = False

        self.block_store = block_store
        self.execution_features = execution_features

        self.spawn_list = []
        
        self.continuation = None
        self.result = None
        
        self.master_proxy = master_proxy
        
        self.is_running = True
        
        self.current_executor = None
        self.exec_result_counter = 0
        self.spawn_exec_counter = 0
        
        # This should be the same as the length of the spawn list, outside 
        # calls to spawn_func and spawn_exec_func.
        self.spawn_counter = 0
        
    def abort(self):
        self.is_running = False
        if self.current_executor is not None:
            try:
                self.current_executor.abort()
            except:
                pass
            
    def fetch_inputs(self, block_store):
        continuation_ref = None
        parsed_inputs = {}
        
        for local_id, ref in self.inputs.items():
            if local_id == '_cont':
                continuation_ref = ref
            else:
                parsed_inputs[int(local_id)] = ref
        
        self.continuation = self.block_store.retrieve_object_for_ref(continuation_ref, 'pickle')

        fetch_objects = []
        for local_id, ref in parsed_inputs.items():
        
            if not self.is_running:
                return
            
            if self.continuation.is_marked_as_dereferenced(local_id):
                if isinstance(ref, SWDataValue):
                    self.continuation.rewrite_reference(local_id, ref)
                else:
                    fetch_objects.append((local_id, ref))
            elif self.continuation.is_marked_as_execd(local_id):
                self.continuation.rewrite_reference(local_id, ref)
            else:
                assert False

        fetch_refs = [ref for (local_id, ref) in fetch_objects]
        fetched_objects = self.block_store.retrieve_objects_for_refs(fetch_refs, 'json')

        for (id, ref, ob) in zip([local_id for (local_id, ref) in fetch_objects], fetch_refs, fetched_objects):
            self.continuation.rewrite_reference(id, SWDataValue(ref.id, ob))
            
        cherrypy.log.error('Fetched all task inputs', 'SWI', logging.INFO)

    def convert_tasklocal_to_real_reference(self, value):
        if isinstance(value, SWLocalReference):
            return self.continuation.resolve_tasklocal_reference_with_ref(value)
        else:
            return value

    def convert_real_to_tasklocal_reference(self, value):
        if isinstance(value, SWRealReference):
            return self.continuation.create_tasklocal_reference(value)
        else:
            return value

    def create_uuid(self):
        if self.replay_uuids:
            ret = self.replay_uuid_list[self.current_uuid_index]
            self.current_uuid_index += 1
        else:
            ret = str(uuid.uuid1())
            self.replay_uuid_list.append(ret)
        return ret

    def interpret(self):
        self.continuation.context.restart()
        task_context = TaskContext(self.continuation.context, self)
        
        task_context.bind_tasklocal_identifier("spawn", LambdaFunction(lambda x: self.spawn_func(x[0], x[1])))
        task_context.bind_tasklocal_identifier("spawn_exec", LambdaFunction(lambda x: self.spawn_exec_func(x[0], x[1], x[2])))
        task_context.bind_tasklocal_identifier("__star__", LambdaFunction(lambda x: self.lazy_dereference(x[0])))
        task_context.bind_tasklocal_identifier("int", SafeLambdaFunction(lambda x: int(x[0]), self))
        task_context.bind_tasklocal_identifier("range", SafeLambdaFunction(lambda x: range(*x), self))
        task_context.bind_tasklocal_identifier("len", SafeLambdaFunction(lambda x: len(x[0]), self))
        task_context.bind_tasklocal_identifier("has_key", SafeLambdaFunction(lambda x: x[1] in x[0], self))
        task_context.bind_tasklocal_identifier("get_key", SafeLambdaFunction(lambda x: x[0][x[1]] if x[1] in x[0] else x[2], self))
        task_context.bind_tasklocal_identifier("exec", LambdaFunction(lambda x: self.exec_func(x[0], x[1], x[2])))
        task_context.bind_tasklocal_identifier("ref", LambdaFunction(lambda x: self.make_reference(x)))
        #task_context.bind_tasklocal_identifier("is_future", LambdaFunction(lambda x: self.is_future(x[0])))
        #task_context.bind_tasklocal_identifier("is_error", LambdaFunction(lambda x: self.is_error(x[0])))
        #task_context.bind_tasklocal_identifier("abort", LambdaFunction(lambda x: self.abort_production(x[0])))
        #task_context.bind_tasklocal_identifier("task_details", LambdaFunction(lambda x: self.get_task_details(x[0])))
        #task_context.bind_tasklocal_identifier("select", LambdaFunction(lambda x: self.select_func(x[0]) if len(x) == 1 else self.select_func(x[0], x[1])))
        visitor = StatementExecutorVisitor(task_context)
        
        try:
            self.result = visitor.visit(self.continuation.task_stmt, self.continuation.stack, 0)
            
            # XXX: This is for the unusual case that we have a task fragment that runs to completion without returning anything.
            #      Could maybe use an ErrorRef here, but this might not be erroneous if, e.g. the interactive shell is used.
            if self.result is None:
                self.result = SWErrorReference(self.expected_outputs[0], 'NO_RETURN_VALUE', 'null')
            
        except SelectException, se:
            
            local_select_group = se.select_group
            timeout = se.timeout
            
            select_group = map(self.continuation.resolve_tasklocal_reference_with_ref, local_select_group)
                        
            cont_task_id = self.create_spawned_task_name()
                        
            cont_task_descriptor = {'task_id': str(cont_task_id),
                                    'handler': 'swi',
                                    'dependencies': {},
                                    'select_group': select_group,
                                    'select_timeout': timeout,
                                    'expected_outputs': map(str, self.expected_outputs),
                                    'save_continuation': self.save_continuation}
            self.save_continuation = False
            self.spawn_list.append(SpawnListEntry(cont_task_id, cont_task_descriptor, self.continuation))
            
        except ExecutionInterruption, ei:

            # Need to add a continuation task to the spawn list.
            cont_deps = {}
            for index in self.continuation.reference_table.keys():
                if (not isinstance(self.continuation.resolve_tasklocal_reference_with_index(index), SWDataValue)) and \
                   (self.continuation.is_marked_as_dereferenced(index) or self.continuation.is_marked_as_execd(index)):
                    cont_deps[index] = self.continuation.resolve_tasklocal_reference_with_index(index)
            cont_task_id = self.create_spawned_task_name()
            cont_task_descriptor = {'task_id': str(cont_task_id),
                                    'handler': 'swi',
                                    'dependencies': cont_deps, # _cont will be added at spawn time.
                                    'expected_outputs': map(str, self.expected_outputs),
                                    'save_continuation': self.save_continuation,
                                    'continues_task': str(self.original_task_id)}
            self.save_continuation = False
            if isinstance(ei, FeatureUnavailableException):
                cont_task_descriptor['require_features'] = [ei.feature_name]
            
            self.spawn_list.append(SpawnListEntry(cont_task_id, cont_task_descriptor, self.continuation))
            return
            
        except MissingInputException as mie:
            cherrypy.log.error('Cannot retrieve inputs for task: %s' % repr(mie.bindings), 'SWI', logging.WARNING)
            raise

        except Exception:
            cherrypy.log.error('Interpreter error.', 'SWI', logging.ERROR, True)
            self.save_continuation = True
            raise

    def spawn_all(self, block_store, master_proxy):
        current_batch = []
        
        if len(self.spawn_list) == 0:
            return
        
        current_index = 0
        while current_index < len(self.spawn_list):
            
            must_wait = False
            
            if self.spawn_list[current_index].ignore:
                current_index += 1
                continue
            
            current_cont = self.spawn_list[current_index].continuation
                
            if must_wait:
                
                if not self.is_running:
                    return
                
                # Fire off the current batch.
                master_proxy.spawn_tasks(self.task_id, current_batch)
                
                # Iterate again on the same index.
                current_batch = []
                continue
                
            else:
                
                # Store the continuation and add it to the task descriptor.
                if current_cont is not None:
                    spawned_cont_id = self.get_spawn_continuation_object_id(self.spawn_list[current_index].id)
                    _, size_hint = block_store.store_object(current_cont, 'pickle', spawned_cont_id)
                    spawned_cont_ref = SW2_ConcreteReference(spawned_cont_id, size_hint)
                    spawned_cont_ref.add_location_hint(self.block_store.netloc)
                    self.spawn_list[current_index].task_descriptor['dependencies']['_cont'] = spawned_cont_ref
                    self.maybe_also_publish(spawned_cont_ref)
            
                # Current task is now ready to be spawned.
                current_batch.append(self.spawn_list[current_index].task_descriptor)
                current_index += 1
            
        if len(current_batch) > 0:
            
            if not self.is_running:
                return
            
            # Fire off the current batch.
            master_proxy.spawn_tasks(self.task_id, current_batch)
            
    def get_spawn_continuation_object_id(self, task_id):
        return '%s:cont' % (task_id, )

    def get_saved_continuation_object_id(self):
        return '%s:saved_cont' % (self.task_id, )

    def maybe_also_publish(self, ref):
        #if self.replay_ref is not None and concrete_ref.id == self.replay_ref.id:
        if isinstance(ref, SW2_ConcreteReference):
            self.additional_refs_to_publish.append(ref)

    def commit_result(self, block_store, master_proxy):
        
        commit_bindings = {}
        for ref in self.additional_refs_to_publish:
            commit_bindings[ref.id] = ref
        
        if self.result is None:
            if self.save_continuation:
                save_cont_uri, _ = self.block_store.store_object(self.continuation, 'pickle', self.get_saved_continuation_object_id())
            else:
                save_cont_uri = None
            master_proxy.commit_task(self.task_id, commit_bindings, save_cont_uri, self.replay_uuid_list)
            return
        
        serializable_result = map_leaf_values(self.convert_tasklocal_to_real_reference, self.result)

        _, size_hint = block_store.store_object(serializable_result, 'json', self.expected_outputs[0])
        if size_hint < 128:
            result_ref = SWDataValue(self.expected_outputs[0], serializable_result)
        else:
            result_ref = SW2_ConcreteReference(self.expected_outputs[0], size_hint)
            result_ref.add_location_hint(self.block_store.netloc)
            
        commit_bindings[self.expected_outputs[0]] = result_ref
        
        if self.save_continuation:
            save_cont_uri, size_hint = self.block_store.store_object(self.continuation, 'pickle', self.get_saved_continuation_object_id())
        else:
            save_cont_uri = None
        
        master_proxy.commit_task(self.task_id, commit_bindings, save_cont_uri, self.replay_uuid_list)

    def build_spawn_continuation(self, spawn_expr, args):
        spawned_task_stmt = ast.Return(ast.SpawnedFunction(spawn_expr, args))
        cont = SWContinuation(spawned_task_stmt, SimpleContext())
        
        # Now need to build the reference table for the spawned task.
        local_reference_indices = set()
        
        # Local references in the arguments.
        for leaf in filter(lambda x: isinstance(x, SWLocalReference), all_leaf_values(args)):
            local_reference_indices.add(leaf.index)
            
        # Local references captured in the lambda/function.
        for leaf in filter(lambda x: isinstance(x, SWLocalReference), all_leaf_values(spawn_expr.captured_bindings)):
            local_reference_indices.add(leaf.index)

        if len(local_reference_indices) > 0:
            cont.current_local_id_index = max(local_reference_indices) + 1

        # Actually build the new reference table.
        # TODO: This would be better if we compressed the table, but might take a while.
        #       So let's assume that we won't run out of indices in a normal run :).
        for index in local_reference_indices:
            cont.reference_table[index] = self.continuation.reference_table[index]
        
        return cont

    def create_spawned_task_name(self):
        sha = hashlib.sha1()
        sha.update('%s:%d' % (self.task_id, self.spawn_counter))
        ret = sha.hexdigest()
        self.spawn_counter += 1
        return ret
    
    def create_spawn_output_name(self, task_id):
        return 'swi:%s' % task_id
    
    def spawn_func(self, spawn_expr, args):

        args = self.do_eager_thunks(args)

        # Create new continuation for the spawned function.
        spawned_continuation = self.build_spawn_continuation(spawn_expr, args)
        
        
        
        # Append the new task definition to the spawn list.
        new_task_id = self.create_spawned_task_name()
        expected_output_id = self.create_spawn_output_name(new_task_id)
        
        # Match up the output with a new tasklocal reference.
        ret = self.continuation.create_tasklocal_reference(SW2_FutureReference(expected_output_id))
        
        task_descriptor = {'task_id': new_task_id,
                           'handler': 'swi',
                           'dependencies': {},
                           'expected_outputs': [str(expected_output_id)] # _cont will be added later
                          }
        
        # TODO: we could visit the spawn expression and try to guess what requirements
        #       and executors we need in here. 
        # TODO: should probably look at dereference wrapper objects in the spawn context
        #       and ship them as inputs.
        
        self.spawn_list.append(SpawnListEntry(new_task_id, task_descriptor, spawned_continuation))

        # Return local reference to the interpreter.
        return ret

    def check_for_eager_fetch(self, leaf):
        if isinstance(leaf, SWDereferenceWrapper):
            real_ref = self.continuation.resolve_tasklocal_reference_with_ref(leaf.ref)
            if isinstance(real_ref, SWURLReference):
                return (leaf.ref.index, real_ref)
            else:
                return None
        else:
            return None

    def do_eager_thunks(self, args):

        fetches_needed = accumulate_leaf_values(self.check_for_eager_fetch, args)
        fetches_needed = filter(lambda x: x is not None, fetches_needed)
        fetched_objects = self.block_store.retrieve_objects_for_refs([ref for (id, ref) in fetches_needed], "json")
        id_to_result_map = dict(zip([id for (id, ref) in fetches_needed], fetched_objects))

        def resolve_thunks_mapper(leaf):
            if isinstance(leaf, SWDereferenceWrapper):
                return self.eager_dereference_from_map(leaf.ref, id_to_result_map)
            else:
                return leaf

        return map_leaf_values(resolve_thunks_mapper, args)

    def spawn_exec_func(self, executor_name, exec_args, num_outputs):
        
        new_task_id = self.create_spawned_task_name()
        inputs = {}
        
        args = self.do_eager_thunks(exec_args)

        args_id, expected_output_ids = self.create_names_for_exec(executor_name, args, num_outputs)
        ret = [self.continuation.create_tasklocal_reference(SW2_FutureReference(expected_output_ids[i])) for i in range(num_outputs)]

        def args_check_mapper(leaf):
            if isinstance(leaf, SWLocalReference):
                real_ref = self.continuation.resolve_tasklocal_reference_with_ref(leaf)
                i = len(inputs)
                inputs[i] = real_ref
                ret = SWLocalReference(i)
                return ret
            return leaf
        
        transformed_args = map_leaf_values(args_check_mapper, args)
        _, size_hint = self.block_store.store_object(transformed_args, 'pickle', args_id)
        args_ref = SW2_ConcreteReference(args_id, size_hint)
        self.spawn_exec_counter += 1
        args_ref.add_location_hint(self.block_store.netloc)
        self.maybe_also_publish(args_ref)
        
        inputs['_args'] = args_ref

        task_descriptor = {'task_id': new_task_id,
                           'handler': executor_name, 
                           'dependencies': inputs,
                           'expected_outputs': expected_output_ids}
        
        self.spawn_list.append(SpawnListEntry(new_task_id, task_descriptor))
        
        if len(self.spawn_list) > 20:
            self.spawn_all(self.block_store, self.master_proxy)
            self.spawn_list = []
        
        return ret
    
    def create_names_for_exec(self, executor_name, real_args, num_outputs):
        sha = hashlib.sha1()
        self.hash_update_with_structure(sha, [real_args, num_outputs])
        prefix = '%s:%s:' % (executor_name, sha.hexdigest())
                
        args_name = '%sargs' % (prefix, )
        output_names = ['%s%d' % (prefix, i) for i in range(num_outputs)]
        return args_name, output_names
    
    def exec_func(self, executor_name, args, num_outputs):
        
        real_args = self.do_eager_thunks(args)

        _, output_ids = self.create_names_for_exec(executor_name, real_args, num_outputs)

        self.current_executor = self.execution_features.get_executor(executor_name, real_args, self.continuation, output_ids, self.master_proxy)
        self.current_executor.execute(self.block_store, self.task_id)
        
        for ref in self.current_executor.output_refs:
            self.exec_result_counter += 1
            self.maybe_also_publish(ref)
        
        ret = map(self.continuation.create_tasklocal_reference, self.current_executor.output_refs)
        self.current_executor = None
        return ret

    def make_reference(self, urls):
        return self.continuation.create_tasklocal_reference(SWURLReference(urls))

    def lazy_dereference(self, ref):
        self.continuation.mark_as_dereferenced(ref)
        return SWDereferenceWrapper(ref)

    def eager_dereference(self, ref):
        real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        if isinstance(real_ref, SWDataValue):
            return map_leaf_values(self.convert_real_to_tasklocal_reference, real_ref.value)
        else:
            self.continuation.mark_as_dereferenced(ref)
            raise ReferenceUnavailableException(ref, self.continuation)

    def eager_dereference_from_map(self, ref, fetches):

        # Any fetches needed by a URL reference should have been performed in advance
        # (see do_eager_thunks)
        
        real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        if isinstance(real_ref, SWDataValue):
            return map_leaf_values(self.convert_real_to_tasklocal_reference, real_ref.value)
        else:
            self.continuation.mark_as_dereferenced(ref)
            raise ReferenceUnavailableException(ref, self.continuation)

    def include_script(self, target_expr):
        if isinstance(target_expr, basestring):
            # Name may be relative to the local stdlib.
            target_expr = urlparse.urljoin('http://%s/stdlib/' % self.block_store.netloc, target_expr)
            target_ref = SWURLReference([target_expr])
        elif isinstance(target_expr, SWLocalReference):    
            target_ref = self.continuation.resolve_tasklocal_reference_with_ref(target_expr)
        else:
            raise BlameUserException('Invalid object %s passed as the argument of include', 'INCLUDE', logging.ERROR)

        try:
            script = self.block_store.retrieve_object_for_ref(target_ref, 'script')
        except:
            cherrypy.log.error('Error parsing included script', 'INCLUDE', logging.ERROR, True)
            raise BlameUserException('The included script did not parse successfully')
        return script

    def is_future(self, ref):
        real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        return isinstance(real_ref, SW2_FutureReference)

    def is_error(self, ref):
        real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        return isinstance(real_ref, SWErrorReference)

    def abort_production(self, ref):
        raise
        #real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        #if isinstance(real_ref, SWLocalFutureReference):
        #    self.spawn_list[real_ref.spawn_list_index].ignore = True
        #elif isinstance(real_ref, SWGlobalFutureReference):
        #    self.master_proxy.abort_production_of_output(real_ref)
        #return True
    
    def get_task_details(self, ref):
        raise
        #real_ref = self.continuation.resolve_tasklocal_reference_with_ref(ref)
        #if isinstance(real_ref, SWGlobalFutureReference):
        #    return self.master_proxy.get_task_details_for_future(ref)
        #else:
        #    return {}

    def select_func(self, select_group, timeout=None):
        if self.select_result is not None:
            return self.select_result
        else:
            raise SelectException(select_group, timeout)

    def hash_update_with_structure(self, hash, value):
        """
        Recurses over a Skywriting data structure (containing lists, dicts and 
        primitive leaves) in a deterministic order, and updates the given hash with
        all values contained therein.
        """
        if isinstance(value, list):
            hash.update('[')
            for element in value:
                self.hash_update_with_structure(hash, element)
                hash.update(',')
            hash.update(']')
        elif isinstance(value, dict):
            hash.update('{')
            for (dict_key, dict_value) in sorted(value.items()):
                hash.update(dict_key)
                hash.update(':')
                self.hash_update_with_structure(hash, dict_value)
                hash.update(',')
            hash.update('}')
        elif isinstance(value, SWLocalReference):
            self.hash_update_with_structure(hash, self.convert_tasklocal_to_real_reference(value))
        elif isinstance(value, SW2_ConcreteReference) or isinstance(value, SW2_FutureReference):
            hash.update('ref')
            hash.update(value.id)
        elif isinstance(value, SWURLReference):
            hash.update('ref')
            hash.update(value.urls[0])
        elif isinstance(value, SWDataValue):
            hash.update('ref*')
            self.hash_update_with_structure(hash, value.value)
        else:
            hash.update(str(value))

def map_leaf_values(f, value):
    """
    Recurses over a Skywriting data structure (containing lists, dicts and 
    primitive leaves), and returns a new structure with the leaves mapped as specified.
    """
    if isinstance(value, list):
        return map(lambda x: map_leaf_values(f, x), value)
    elif isinstance(value, dict):
        ret = {}
        for (dict_key, dict_value) in value.items():
            key = map_leaf_values(f, dict_key)
            value = map_leaf_values(f, dict_value)
            ret[key] = value
        return ret
    else:
        return f(value)

def accumulate_leaf_values(f, value):

    def flatten_lofl(ls):
        ret = []
        for l in ls:
            ret.extend(l)
        return ret

    if isinstance(value, list):
        accumd_list = [accumulate_leaf_values(f, v) for v in value]
        return flatten_lofl(accumd_list)
    elif isinstance(value, dict):
        accumd_keys = flatten_lofl([accumulate_leaf_values(f, v) for v in value.keys()])
        accumd_values = flatten_lofl([accumulate_leaf_values(f, v) for v in value.values()])
        accumd_keys.extend(accumd_values)
        return accumd_keys
    else:
        return [f(value)]
