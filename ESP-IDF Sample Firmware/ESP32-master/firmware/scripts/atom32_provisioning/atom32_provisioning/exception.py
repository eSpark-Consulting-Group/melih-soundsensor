class ProvisioningException(Exception):
    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)

class RuntimeException(ProvisioningException):
    def __init__(self,*args,**kwargs):
        ProvisioningException.__init__(self,*args,**kwargs)

class TimeoutException(RuntimeException):
    def __init__(self,*args,**kwargs):
        RuntimeException.__init__(self,*args,**kwargs)

class RebootException(RuntimeException):
    def __init__(self,*args,**kwargs):
        RuntimeException.__init__(self,*args,**kwargs)
