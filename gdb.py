SIGSEGV = 0x10
SIGBUS = 5
import logging
logger = logging.getLogger('gdb_script')
logger.setLevel(logging.DEBUG)
# create file handler which logs even debug messages
fh = logging.FileHandler('spam.log')
fh.setLevel(logging.DEBUG)
# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.ERROR)
# create formatter and add it to the handlers
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
# add the handlers to the logger
logger.addHandler(fh)
logger.addHandler(ch)

class Executor:
    def __init__(self, cmd):
        self.__cmd = cmd

    def __call__(self):
        gdb.execute(self.__cmd)

def info(msg):
    print(f"[ME] {msg}")
    logger.info(msg)

def event_handler(event):
    # gdb.execute("set scheduler-locking on") # to avoid parallel signals in other threads
    info(f"SIG {event.stop_signal}")
    t = gdb.selected_thread()
    if t.name == "CPU thread" and (event.stop_signal == "SIGSEGV"):
        info("on cpu thread, everything ok!")
    else:
        info(f"on thread {t.name}, not ok!!!")
        return
    # gdb.execute("set scheduler-locking off") # otherwise just this thread is continued, leading to a deadlock   
    # gdb.execute("signal " + str(event.stop_signal))
    gdb.post_event(Executor("signal " + str(event.stop_signal))) # and post the continue command to gdb


gdb.execute("set logging on")
gdb.execute("set logging file gdbout")
gdb.execute("set pagination off")
gdb.events.stop.connect(event_handler)