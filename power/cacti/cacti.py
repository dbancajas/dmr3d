# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _cacti

def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name) or (name == "thisown"):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


CHUNKSIZE = _cacti.CHUNKSIZE
class powerComponents(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, powerComponents, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, powerComponents, name)
    def __repr__(self):
        return "<%s.%s; proxy of C powerComponents instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["dynamic"] = _cacti.powerComponents_dynamic_set
    __swig_getmethods__["dynamic"] = _cacti.powerComponents_dynamic_get
    if _newclass:dynamic = property(_cacti.powerComponents_dynamic_get, _cacti.powerComponents_dynamic_set)
    __swig_setmethods__["leakage"] = _cacti.powerComponents_leakage_set
    __swig_getmethods__["leakage"] = _cacti.powerComponents_leakage_get
    if _newclass:leakage = property(_cacti.powerComponents_leakage_get, _cacti.powerComponents_leakage_set)
    def __init__(self, *args):
        _swig_setattr(self, powerComponents, 'this', _cacti.new_powerComponents(*args))
        _swig_setattr(self, powerComponents, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_powerComponents):
        try:
            if self.thisown: destroy(self)
        except: pass


class powerComponentsPtr(powerComponents):
    def __init__(self, this):
        _swig_setattr(self, powerComponents, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, powerComponents, 'thisown', 0)
        _swig_setattr(self, powerComponents,self.__class__,powerComponents)
_cacti.powerComponents_swigregister(powerComponentsPtr)

class powerDef(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, powerDef, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, powerDef, name)
    def __repr__(self):
        return "<%s.%s; proxy of C powerDef instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["readOp"] = _cacti.powerDef_readOp_set
    __swig_getmethods__["readOp"] = _cacti.powerDef_readOp_get
    if _newclass:readOp = property(_cacti.powerDef_readOp_get, _cacti.powerDef_readOp_set)
    __swig_setmethods__["writeOp"] = _cacti.powerDef_writeOp_set
    __swig_getmethods__["writeOp"] = _cacti.powerDef_writeOp_get
    if _newclass:writeOp = property(_cacti.powerDef_writeOp_get, _cacti.powerDef_writeOp_set)
    def __init__(self, *args):
        _swig_setattr(self, powerDef, 'this', _cacti.new_powerDef(*args))
        _swig_setattr(self, powerDef, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_powerDef):
        try:
            if self.thisown: destroy(self)
        except: pass


class powerDefPtr(powerDef):
    def __init__(self, this):
        _swig_setattr(self, powerDef, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, powerDef, 'thisown', 0)
        _swig_setattr(self, powerDef,self.__class__,powerDef)
_cacti.powerDef_swigregister(powerDefPtr)

class cache_params_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, cache_params_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, cache_params_t, name)
    def __repr__(self):
        return "<%s.%s; proxy of C cache_params_t instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["nsets"] = _cacti.cache_params_t_nsets_set
    __swig_getmethods__["nsets"] = _cacti.cache_params_t_nsets_get
    if _newclass:nsets = property(_cacti.cache_params_t_nsets_get, _cacti.cache_params_t_nsets_set)
    __swig_setmethods__["assoc"] = _cacti.cache_params_t_assoc_set
    __swig_getmethods__["assoc"] = _cacti.cache_params_t_assoc_get
    if _newclass:assoc = property(_cacti.cache_params_t_assoc_get, _cacti.cache_params_t_assoc_set)
    __swig_setmethods__["dbits"] = _cacti.cache_params_t_dbits_set
    __swig_getmethods__["dbits"] = _cacti.cache_params_t_dbits_get
    if _newclass:dbits = property(_cacti.cache_params_t_dbits_get, _cacti.cache_params_t_dbits_set)
    __swig_setmethods__["tbits"] = _cacti.cache_params_t_tbits_set
    __swig_getmethods__["tbits"] = _cacti.cache_params_t_tbits_get
    if _newclass:tbits = property(_cacti.cache_params_t_tbits_get, _cacti.cache_params_t_tbits_set)
    __swig_setmethods__["nbanks"] = _cacti.cache_params_t_nbanks_set
    __swig_getmethods__["nbanks"] = _cacti.cache_params_t_nbanks_get
    if _newclass:nbanks = property(_cacti.cache_params_t_nbanks_get, _cacti.cache_params_t_nbanks_set)
    __swig_setmethods__["rport"] = _cacti.cache_params_t_rport_set
    __swig_getmethods__["rport"] = _cacti.cache_params_t_rport_get
    if _newclass:rport = property(_cacti.cache_params_t_rport_get, _cacti.cache_params_t_rport_set)
    __swig_setmethods__["wport"] = _cacti.cache_params_t_wport_set
    __swig_getmethods__["wport"] = _cacti.cache_params_t_wport_get
    if _newclass:wport = property(_cacti.cache_params_t_wport_get, _cacti.cache_params_t_wport_set)
    __swig_setmethods__["rwport"] = _cacti.cache_params_t_rwport_set
    __swig_getmethods__["rwport"] = _cacti.cache_params_t_rwport_get
    if _newclass:rwport = property(_cacti.cache_params_t_rwport_get, _cacti.cache_params_t_rwport_set)
    __swig_setmethods__["serport"] = _cacti.cache_params_t_serport_set
    __swig_getmethods__["serport"] = _cacti.cache_params_t_serport_get
    if _newclass:serport = property(_cacti.cache_params_t_serport_get, _cacti.cache_params_t_serport_set)
    __swig_setmethods__["obits"] = _cacti.cache_params_t_obits_set
    __swig_getmethods__["obits"] = _cacti.cache_params_t_obits_get
    if _newclass:obits = property(_cacti.cache_params_t_obits_get, _cacti.cache_params_t_obits_set)
    __swig_setmethods__["abits"] = _cacti.cache_params_t_abits_set
    __swig_getmethods__["abits"] = _cacti.cache_params_t_abits_get
    if _newclass:abits = property(_cacti.cache_params_t_abits_get, _cacti.cache_params_t_abits_set)
    __swig_setmethods__["dweight"] = _cacti.cache_params_t_dweight_set
    __swig_getmethods__["dweight"] = _cacti.cache_params_t_dweight_get
    if _newclass:dweight = property(_cacti.cache_params_t_dweight_get, _cacti.cache_params_t_dweight_set)
    __swig_setmethods__["pweight"] = _cacti.cache_params_t_pweight_set
    __swig_getmethods__["pweight"] = _cacti.cache_params_t_pweight_get
    if _newclass:pweight = property(_cacti.cache_params_t_pweight_get, _cacti.cache_params_t_pweight_set)
    __swig_setmethods__["aweight"] = _cacti.cache_params_t_aweight_set
    __swig_getmethods__["aweight"] = _cacti.cache_params_t_aweight_get
    if _newclass:aweight = property(_cacti.cache_params_t_aweight_get, _cacti.cache_params_t_aweight_set)
    def __init__(self, *args):
        _swig_setattr(self, cache_params_t, 'this', _cacti.new_cache_params_t(*args))
        _swig_setattr(self, cache_params_t, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_cache_params_t):
        try:
            if self.thisown: destroy(self)
        except: pass


class cache_params_tPtr(cache_params_t):
    def __init__(self, this):
        _swig_setattr(self, cache_params_t, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, cache_params_t, 'thisown', 0)
        _swig_setattr(self, cache_params_t,self.__class__,cache_params_t)
_cacti.cache_params_t_swigregister(cache_params_tPtr)

class subarray_params_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, subarray_params_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, subarray_params_t, name)
    def __repr__(self):
        return "<%s.%s; proxy of C subarray_params_t instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["Ndwl"] = _cacti.subarray_params_t_Ndwl_set
    __swig_getmethods__["Ndwl"] = _cacti.subarray_params_t_Ndwl_get
    if _newclass:Ndwl = property(_cacti.subarray_params_t_Ndwl_get, _cacti.subarray_params_t_Ndwl_set)
    __swig_setmethods__["Ndbl"] = _cacti.subarray_params_t_Ndbl_set
    __swig_getmethods__["Ndbl"] = _cacti.subarray_params_t_Ndbl_get
    if _newclass:Ndbl = property(_cacti.subarray_params_t_Ndbl_get, _cacti.subarray_params_t_Ndbl_set)
    __swig_setmethods__["Nspd"] = _cacti.subarray_params_t_Nspd_set
    __swig_getmethods__["Nspd"] = _cacti.subarray_params_t_Nspd_get
    if _newclass:Nspd = property(_cacti.subarray_params_t_Nspd_get, _cacti.subarray_params_t_Nspd_set)
    __swig_setmethods__["Ntwl"] = _cacti.subarray_params_t_Ntwl_set
    __swig_getmethods__["Ntwl"] = _cacti.subarray_params_t_Ntwl_get
    if _newclass:Ntwl = property(_cacti.subarray_params_t_Ntwl_get, _cacti.subarray_params_t_Ntwl_set)
    __swig_setmethods__["Ntbl"] = _cacti.subarray_params_t_Ntbl_set
    __swig_getmethods__["Ntbl"] = _cacti.subarray_params_t_Ntbl_get
    if _newclass:Ntbl = property(_cacti.subarray_params_t_Ntbl_get, _cacti.subarray_params_t_Ntbl_set)
    __swig_setmethods__["Ntspd"] = _cacti.subarray_params_t_Ntspd_set
    __swig_getmethods__["Ntspd"] = _cacti.subarray_params_t_Ntspd_get
    if _newclass:Ntspd = property(_cacti.subarray_params_t_Ntspd_get, _cacti.subarray_params_t_Ntspd_set)
    __swig_setmethods__["muxover"] = _cacti.subarray_params_t_muxover_set
    __swig_getmethods__["muxover"] = _cacti.subarray_params_t_muxover_get
    if _newclass:muxover = property(_cacti.subarray_params_t_muxover_get, _cacti.subarray_params_t_muxover_set)
    def __init__(self, *args):
        _swig_setattr(self, subarray_params_t, 'this', _cacti.new_subarray_params_t(*args))
        _swig_setattr(self, subarray_params_t, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_subarray_params_t):
        try:
            if self.thisown: destroy(self)
        except: pass


class subarray_params_tPtr(subarray_params_t):
    def __init__(self, this):
        _swig_setattr(self, subarray_params_t, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, subarray_params_t, 'thisown', 0)
        _swig_setattr(self, subarray_params_t,self.__class__,subarray_params_t)
_cacti.subarray_params_t_swigregister(subarray_params_tPtr)

class tech_params_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, tech_params_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, tech_params_t, name)
    def __repr__(self):
        return "<%s.%s; proxy of C tech_params_t instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["tech_size"] = _cacti.tech_params_t_tech_size_set
    __swig_getmethods__["tech_size"] = _cacti.tech_params_t_tech_size_get
    if _newclass:tech_size = property(_cacti.tech_params_t_tech_size_get, _cacti.tech_params_t_tech_size_set)
    __swig_setmethods__["crossover"] = _cacti.tech_params_t_crossover_set
    __swig_getmethods__["crossover"] = _cacti.tech_params_t_crossover_get
    if _newclass:crossover = property(_cacti.tech_params_t_crossover_get, _cacti.tech_params_t_crossover_set)
    __swig_setmethods__["standby"] = _cacti.tech_params_t_standby_set
    __swig_getmethods__["standby"] = _cacti.tech_params_t_standby_get
    if _newclass:standby = property(_cacti.tech_params_t_standby_get, _cacti.tech_params_t_standby_set)
    __swig_setmethods__["VddPow"] = _cacti.tech_params_t_VddPow_set
    __swig_getmethods__["VddPow"] = _cacti.tech_params_t_VddPow_get
    if _newclass:VddPow = property(_cacti.tech_params_t_VddPow_get, _cacti.tech_params_t_VddPow_set)
    __swig_setmethods__["scaling_factor"] = _cacti.tech_params_t_scaling_factor_set
    __swig_getmethods__["scaling_factor"] = _cacti.tech_params_t_scaling_factor_get
    if _newclass:scaling_factor = property(_cacti.tech_params_t_scaling_factor_get, _cacti.tech_params_t_scaling_factor_set)
    def __init__(self, *args):
        _swig_setattr(self, tech_params_t, 'this', _cacti.new_tech_params_t(*args))
        _swig_setattr(self, tech_params_t, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_tech_params_t):
        try:
            if self.thisown: destroy(self)
        except: pass


class tech_params_tPtr(tech_params_t):
    def __init__(self, this):
        _swig_setattr(self, tech_params_t, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, tech_params_t, 'thisown', 0)
        _swig_setattr(self, tech_params_t,self.__class__,tech_params_t)
_cacti.tech_params_t_swigregister(tech_params_tPtr)

class area_type(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, area_type, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, area_type, name)
    def __repr__(self):
        return "<%s.%s; proxy of C area_type instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["height"] = _cacti.area_type_height_set
    __swig_getmethods__["height"] = _cacti.area_type_height_get
    if _newclass:height = property(_cacti.area_type_height_get, _cacti.area_type_height_set)
    __swig_setmethods__["width"] = _cacti.area_type_width_set
    __swig_getmethods__["width"] = _cacti.area_type_width_get
    if _newclass:width = property(_cacti.area_type_width_get, _cacti.area_type_width_set)
    __swig_setmethods__["scaled_area"] = _cacti.area_type_scaled_area_set
    __swig_getmethods__["scaled_area"] = _cacti.area_type_scaled_area_get
    if _newclass:scaled_area = property(_cacti.area_type_scaled_area_get, _cacti.area_type_scaled_area_set)
    def __init__(self, *args):
        _swig_setattr(self, area_type, 'this', _cacti.new_area_type(*args))
        _swig_setattr(self, area_type, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_area_type):
        try:
            if self.thisown: destroy(self)
        except: pass


class area_typePtr(area_type):
    def __init__(self, this):
        _swig_setattr(self, area_type, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, area_type, 'thisown', 0)
        _swig_setattr(self, area_type,self.__class__,area_type)
_cacti.area_type_swigregister(area_typePtr)

class arearesult_type(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, arearesult_type, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, arearesult_type, name)
    def __repr__(self):
        return "<%s.%s; proxy of C arearesult_type instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["dataarray_area"] = _cacti.arearesult_type_dataarray_area_set
    __swig_getmethods__["dataarray_area"] = _cacti.arearesult_type_dataarray_area_get
    if _newclass:dataarray_area = property(_cacti.arearesult_type_dataarray_area_get, _cacti.arearesult_type_dataarray_area_set)
    __swig_setmethods__["datapredecode_area"] = _cacti.arearesult_type_datapredecode_area_set
    __swig_getmethods__["datapredecode_area"] = _cacti.arearesult_type_datapredecode_area_get
    if _newclass:datapredecode_area = property(_cacti.arearesult_type_datapredecode_area_get, _cacti.arearesult_type_datapredecode_area_set)
    __swig_setmethods__["datacolmuxpredecode_area"] = _cacti.arearesult_type_datacolmuxpredecode_area_set
    __swig_getmethods__["datacolmuxpredecode_area"] = _cacti.arearesult_type_datacolmuxpredecode_area_get
    if _newclass:datacolmuxpredecode_area = property(_cacti.arearesult_type_datacolmuxpredecode_area_get, _cacti.arearesult_type_datacolmuxpredecode_area_set)
    __swig_setmethods__["datacolmuxpostdecode_area"] = _cacti.arearesult_type_datacolmuxpostdecode_area_set
    __swig_getmethods__["datacolmuxpostdecode_area"] = _cacti.arearesult_type_datacolmuxpostdecode_area_get
    if _newclass:datacolmuxpostdecode_area = property(_cacti.arearesult_type_datacolmuxpostdecode_area_get, _cacti.arearesult_type_datacolmuxpostdecode_area_set)
    __swig_setmethods__["datawritesig_area"] = _cacti.arearesult_type_datawritesig_area_set
    __swig_getmethods__["datawritesig_area"] = _cacti.arearesult_type_datawritesig_area_get
    if _newclass:datawritesig_area = property(_cacti.arearesult_type_datawritesig_area_get, _cacti.arearesult_type_datawritesig_area_set)
    __swig_setmethods__["tagarray_area"] = _cacti.arearesult_type_tagarray_area_set
    __swig_getmethods__["tagarray_area"] = _cacti.arearesult_type_tagarray_area_get
    if _newclass:tagarray_area = property(_cacti.arearesult_type_tagarray_area_get, _cacti.arearesult_type_tagarray_area_set)
    __swig_setmethods__["tagpredecode_area"] = _cacti.arearesult_type_tagpredecode_area_set
    __swig_getmethods__["tagpredecode_area"] = _cacti.arearesult_type_tagpredecode_area_get
    if _newclass:tagpredecode_area = property(_cacti.arearesult_type_tagpredecode_area_get, _cacti.arearesult_type_tagpredecode_area_set)
    __swig_setmethods__["tagcolmuxpredecode_area"] = _cacti.arearesult_type_tagcolmuxpredecode_area_set
    __swig_getmethods__["tagcolmuxpredecode_area"] = _cacti.arearesult_type_tagcolmuxpredecode_area_get
    if _newclass:tagcolmuxpredecode_area = property(_cacti.arearesult_type_tagcolmuxpredecode_area_get, _cacti.arearesult_type_tagcolmuxpredecode_area_set)
    __swig_setmethods__["tagcolmuxpostdecode_area"] = _cacti.arearesult_type_tagcolmuxpostdecode_area_set
    __swig_getmethods__["tagcolmuxpostdecode_area"] = _cacti.arearesult_type_tagcolmuxpostdecode_area_get
    if _newclass:tagcolmuxpostdecode_area = property(_cacti.arearesult_type_tagcolmuxpostdecode_area_get, _cacti.arearesult_type_tagcolmuxpostdecode_area_set)
    __swig_setmethods__["tagoutdrvdecode_area"] = _cacti.arearesult_type_tagoutdrvdecode_area_set
    __swig_getmethods__["tagoutdrvdecode_area"] = _cacti.arearesult_type_tagoutdrvdecode_area_get
    if _newclass:tagoutdrvdecode_area = property(_cacti.arearesult_type_tagoutdrvdecode_area_get, _cacti.arearesult_type_tagoutdrvdecode_area_set)
    __swig_setmethods__["tagoutdrvsig_area"] = _cacti.arearesult_type_tagoutdrvsig_area_set
    __swig_getmethods__["tagoutdrvsig_area"] = _cacti.arearesult_type_tagoutdrvsig_area_get
    if _newclass:tagoutdrvsig_area = property(_cacti.arearesult_type_tagoutdrvsig_area_get, _cacti.arearesult_type_tagoutdrvsig_area_set)
    __swig_setmethods__["totalarea"] = _cacti.arearesult_type_totalarea_set
    __swig_getmethods__["totalarea"] = _cacti.arearesult_type_totalarea_get
    if _newclass:totalarea = property(_cacti.arearesult_type_totalarea_get, _cacti.arearesult_type_totalarea_set)
    __swig_setmethods__["subbankarea"] = _cacti.arearesult_type_subbankarea_set
    __swig_getmethods__["subbankarea"] = _cacti.arearesult_type_subbankarea_get
    if _newclass:subbankarea = property(_cacti.arearesult_type_subbankarea_get, _cacti.arearesult_type_subbankarea_set)
    __swig_setmethods__["total_dataarea"] = _cacti.arearesult_type_total_dataarea_set
    __swig_getmethods__["total_dataarea"] = _cacti.arearesult_type_total_dataarea_get
    if _newclass:total_dataarea = property(_cacti.arearesult_type_total_dataarea_get, _cacti.arearesult_type_total_dataarea_set)
    __swig_setmethods__["total_tagarea"] = _cacti.arearesult_type_total_tagarea_set
    __swig_getmethods__["total_tagarea"] = _cacti.arearesult_type_total_tagarea_get
    if _newclass:total_tagarea = property(_cacti.arearesult_type_total_tagarea_get, _cacti.arearesult_type_total_tagarea_set)
    __swig_setmethods__["max_efficiency"] = _cacti.arearesult_type_max_efficiency_set
    __swig_getmethods__["max_efficiency"] = _cacti.arearesult_type_max_efficiency_get
    if _newclass:max_efficiency = property(_cacti.arearesult_type_max_efficiency_get, _cacti.arearesult_type_max_efficiency_set)
    __swig_setmethods__["efficiency"] = _cacti.arearesult_type_efficiency_set
    __swig_getmethods__["efficiency"] = _cacti.arearesult_type_efficiency_get
    if _newclass:efficiency = property(_cacti.arearesult_type_efficiency_get, _cacti.arearesult_type_efficiency_set)
    __swig_setmethods__["max_aspect_ratio_total"] = _cacti.arearesult_type_max_aspect_ratio_total_set
    __swig_getmethods__["max_aspect_ratio_total"] = _cacti.arearesult_type_max_aspect_ratio_total_get
    if _newclass:max_aspect_ratio_total = property(_cacti.arearesult_type_max_aspect_ratio_total_get, _cacti.arearesult_type_max_aspect_ratio_total_set)
    __swig_setmethods__["aspect_ratio_total"] = _cacti.arearesult_type_aspect_ratio_total_set
    __swig_getmethods__["aspect_ratio_total"] = _cacti.arearesult_type_aspect_ratio_total_get
    if _newclass:aspect_ratio_total = property(_cacti.arearesult_type_aspect_ratio_total_get, _cacti.arearesult_type_aspect_ratio_total_set)
    __swig_setmethods__["perc_data"] = _cacti.arearesult_type_perc_data_set
    __swig_getmethods__["perc_data"] = _cacti.arearesult_type_perc_data_get
    if _newclass:perc_data = property(_cacti.arearesult_type_perc_data_get, _cacti.arearesult_type_perc_data_set)
    __swig_setmethods__["perc_tag"] = _cacti.arearesult_type_perc_tag_set
    __swig_getmethods__["perc_tag"] = _cacti.arearesult_type_perc_tag_get
    if _newclass:perc_tag = property(_cacti.arearesult_type_perc_tag_get, _cacti.arearesult_type_perc_tag_set)
    __swig_setmethods__["perc_cont"] = _cacti.arearesult_type_perc_cont_set
    __swig_getmethods__["perc_cont"] = _cacti.arearesult_type_perc_cont_get
    if _newclass:perc_cont = property(_cacti.arearesult_type_perc_cont_get, _cacti.arearesult_type_perc_cont_set)
    __swig_setmethods__["sub_eff"] = _cacti.arearesult_type_sub_eff_set
    __swig_getmethods__["sub_eff"] = _cacti.arearesult_type_sub_eff_get
    if _newclass:sub_eff = property(_cacti.arearesult_type_sub_eff_get, _cacti.arearesult_type_sub_eff_set)
    __swig_setmethods__["total_eff"] = _cacti.arearesult_type_total_eff_set
    __swig_getmethods__["total_eff"] = _cacti.arearesult_type_total_eff_get
    if _newclass:total_eff = property(_cacti.arearesult_type_total_eff_get, _cacti.arearesult_type_total_eff_set)
    def __init__(self, *args):
        _swig_setattr(self, arearesult_type, 'this', _cacti.new_arearesult_type(*args))
        _swig_setattr(self, arearesult_type, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_arearesult_type):
        try:
            if self.thisown: destroy(self)
        except: pass


class arearesult_typePtr(arearesult_type):
    def __init__(self, this):
        _swig_setattr(self, arearesult_type, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, arearesult_type, 'thisown', 0)
        _swig_setattr(self, arearesult_type,self.__class__,arearesult_type)
_cacti.arearesult_type_swigregister(arearesult_typePtr)

class parameter_type(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, parameter_type, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, parameter_type, name)
    def __repr__(self):
        return "<%s.%s; proxy of C parameter_type instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["cache_size"] = _cacti.parameter_type_cache_size_set
    __swig_getmethods__["cache_size"] = _cacti.parameter_type_cache_size_get
    if _newclass:cache_size = property(_cacti.parameter_type_cache_size_get, _cacti.parameter_type_cache_size_set)
    __swig_setmethods__["number_of_sets"] = _cacti.parameter_type_number_of_sets_set
    __swig_getmethods__["number_of_sets"] = _cacti.parameter_type_number_of_sets_get
    if _newclass:number_of_sets = property(_cacti.parameter_type_number_of_sets_get, _cacti.parameter_type_number_of_sets_set)
    __swig_setmethods__["tag_associativity"] = _cacti.parameter_type_tag_associativity_set
    __swig_getmethods__["tag_associativity"] = _cacti.parameter_type_tag_associativity_get
    if _newclass:tag_associativity = property(_cacti.parameter_type_tag_associativity_get, _cacti.parameter_type_tag_associativity_set)
    __swig_setmethods__["data_associativity"] = _cacti.parameter_type_data_associativity_set
    __swig_getmethods__["data_associativity"] = _cacti.parameter_type_data_associativity_get
    if _newclass:data_associativity = property(_cacti.parameter_type_data_associativity_get, _cacti.parameter_type_data_associativity_set)
    __swig_setmethods__["block_size"] = _cacti.parameter_type_block_size_set
    __swig_getmethods__["block_size"] = _cacti.parameter_type_block_size_get
    if _newclass:block_size = property(_cacti.parameter_type_block_size_get, _cacti.parameter_type_block_size_set)
    __swig_setmethods__["num_write_ports"] = _cacti.parameter_type_num_write_ports_set
    __swig_getmethods__["num_write_ports"] = _cacti.parameter_type_num_write_ports_get
    if _newclass:num_write_ports = property(_cacti.parameter_type_num_write_ports_get, _cacti.parameter_type_num_write_ports_set)
    __swig_setmethods__["num_readwrite_ports"] = _cacti.parameter_type_num_readwrite_ports_set
    __swig_getmethods__["num_readwrite_ports"] = _cacti.parameter_type_num_readwrite_ports_get
    if _newclass:num_readwrite_ports = property(_cacti.parameter_type_num_readwrite_ports_get, _cacti.parameter_type_num_readwrite_ports_set)
    __swig_setmethods__["num_read_ports"] = _cacti.parameter_type_num_read_ports_set
    __swig_getmethods__["num_read_ports"] = _cacti.parameter_type_num_read_ports_get
    if _newclass:num_read_ports = property(_cacti.parameter_type_num_read_ports_get, _cacti.parameter_type_num_read_ports_set)
    __swig_setmethods__["num_single_ended_read_ports"] = _cacti.parameter_type_num_single_ended_read_ports_set
    __swig_getmethods__["num_single_ended_read_ports"] = _cacti.parameter_type_num_single_ended_read_ports_get
    if _newclass:num_single_ended_read_ports = property(_cacti.parameter_type_num_single_ended_read_ports_get, _cacti.parameter_type_num_single_ended_read_ports_set)
    __swig_setmethods__["fully_assoc"] = _cacti.parameter_type_fully_assoc_set
    __swig_getmethods__["fully_assoc"] = _cacti.parameter_type_fully_assoc_get
    if _newclass:fully_assoc = property(_cacti.parameter_type_fully_assoc_get, _cacti.parameter_type_fully_assoc_set)
    __swig_setmethods__["fudgefactor"] = _cacti.parameter_type_fudgefactor_set
    __swig_getmethods__["fudgefactor"] = _cacti.parameter_type_fudgefactor_get
    if _newclass:fudgefactor = property(_cacti.parameter_type_fudgefactor_get, _cacti.parameter_type_fudgefactor_set)
    __swig_setmethods__["tech_size"] = _cacti.parameter_type_tech_size_set
    __swig_getmethods__["tech_size"] = _cacti.parameter_type_tech_size_get
    if _newclass:tech_size = property(_cacti.parameter_type_tech_size_get, _cacti.parameter_type_tech_size_set)
    __swig_setmethods__["VddPow"] = _cacti.parameter_type_VddPow_set
    __swig_getmethods__["VddPow"] = _cacti.parameter_type_VddPow_get
    if _newclass:VddPow = property(_cacti.parameter_type_VddPow_get, _cacti.parameter_type_VddPow_set)
    __swig_setmethods__["sequential_access"] = _cacti.parameter_type_sequential_access_set
    __swig_getmethods__["sequential_access"] = _cacti.parameter_type_sequential_access_get
    if _newclass:sequential_access = property(_cacti.parameter_type_sequential_access_get, _cacti.parameter_type_sequential_access_set)
    __swig_setmethods__["fast_access"] = _cacti.parameter_type_fast_access_set
    __swig_getmethods__["fast_access"] = _cacti.parameter_type_fast_access_get
    if _newclass:fast_access = property(_cacti.parameter_type_fast_access_get, _cacti.parameter_type_fast_access_set)
    __swig_setmethods__["force_tag"] = _cacti.parameter_type_force_tag_set
    __swig_getmethods__["force_tag"] = _cacti.parameter_type_force_tag_get
    if _newclass:force_tag = property(_cacti.parameter_type_force_tag_get, _cacti.parameter_type_force_tag_set)
    __swig_setmethods__["tag_size"] = _cacti.parameter_type_tag_size_set
    __swig_getmethods__["tag_size"] = _cacti.parameter_type_tag_size_get
    if _newclass:tag_size = property(_cacti.parameter_type_tag_size_get, _cacti.parameter_type_tag_size_set)
    __swig_setmethods__["nr_bits_out"] = _cacti.parameter_type_nr_bits_out_set
    __swig_getmethods__["nr_bits_out"] = _cacti.parameter_type_nr_bits_out_get
    if _newclass:nr_bits_out = property(_cacti.parameter_type_nr_bits_out_get, _cacti.parameter_type_nr_bits_out_set)
    __swig_setmethods__["pure_sram"] = _cacti.parameter_type_pure_sram_set
    __swig_getmethods__["pure_sram"] = _cacti.parameter_type_pure_sram_get
    if _newclass:pure_sram = property(_cacti.parameter_type_pure_sram_get, _cacti.parameter_type_pure_sram_set)
    def __init__(self, *args):
        _swig_setattr(self, parameter_type, 'this', _cacti.new_parameter_type(*args))
        _swig_setattr(self, parameter_type, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_parameter_type):
        try:
            if self.thisown: destroy(self)
        except: pass


class parameter_typePtr(parameter_type):
    def __init__(self, this):
        _swig_setattr(self, parameter_type, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, parameter_type, 'thisown', 0)
        _swig_setattr(self, parameter_type,self.__class__,parameter_type)
_cacti.parameter_type_swigregister(parameter_typePtr)

class result_type(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, result_type, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, result_type, name)
    def __repr__(self):
        return "<%s.%s; proxy of C result_type instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["subbanks"] = _cacti.result_type_subbanks_set
    __swig_getmethods__["subbanks"] = _cacti.result_type_subbanks_get
    if _newclass:subbanks = property(_cacti.result_type_subbanks_get, _cacti.result_type_subbanks_set)
    __swig_setmethods__["access_time"] = _cacti.result_type_access_time_set
    __swig_getmethods__["access_time"] = _cacti.result_type_access_time_get
    if _newclass:access_time = property(_cacti.result_type_access_time_get, _cacti.result_type_access_time_set)
    __swig_setmethods__["cycle_time"] = _cacti.result_type_cycle_time_set
    __swig_getmethods__["cycle_time"] = _cacti.result_type_cycle_time_get
    if _newclass:cycle_time = property(_cacti.result_type_cycle_time_get, _cacti.result_type_cycle_time_set)
    __swig_setmethods__["senseext_scale"] = _cacti.result_type_senseext_scale_set
    __swig_getmethods__["senseext_scale"] = _cacti.result_type_senseext_scale_get
    if _newclass:senseext_scale = property(_cacti.result_type_senseext_scale_get, _cacti.result_type_senseext_scale_set)
    __swig_setmethods__["total_power"] = _cacti.result_type_total_power_set
    __swig_getmethods__["total_power"] = _cacti.result_type_total_power_get
    if _newclass:total_power = property(_cacti.result_type_total_power_get, _cacti.result_type_total_power_set)
    __swig_setmethods__["best_Ndwl"] = _cacti.result_type_best_Ndwl_set
    __swig_getmethods__["best_Ndwl"] = _cacti.result_type_best_Ndwl_get
    if _newclass:best_Ndwl = property(_cacti.result_type_best_Ndwl_get, _cacti.result_type_best_Ndwl_set)
    __swig_setmethods__["best_Ndbl"] = _cacti.result_type_best_Ndbl_set
    __swig_getmethods__["best_Ndbl"] = _cacti.result_type_best_Ndbl_get
    if _newclass:best_Ndbl = property(_cacti.result_type_best_Ndbl_get, _cacti.result_type_best_Ndbl_set)
    __swig_setmethods__["max_power"] = _cacti.result_type_max_power_set
    __swig_getmethods__["max_power"] = _cacti.result_type_max_power_get
    if _newclass:max_power = property(_cacti.result_type_max_power_get, _cacti.result_type_max_power_set)
    __swig_setmethods__["max_access_time"] = _cacti.result_type_max_access_time_set
    __swig_getmethods__["max_access_time"] = _cacti.result_type_max_access_time_get
    if _newclass:max_access_time = property(_cacti.result_type_max_access_time_get, _cacti.result_type_max_access_time_set)
    __swig_setmethods__["best_Nspd"] = _cacti.result_type_best_Nspd_set
    __swig_getmethods__["best_Nspd"] = _cacti.result_type_best_Nspd_get
    if _newclass:best_Nspd = property(_cacti.result_type_best_Nspd_get, _cacti.result_type_best_Nspd_set)
    __swig_setmethods__["best_Ntwl"] = _cacti.result_type_best_Ntwl_set
    __swig_getmethods__["best_Ntwl"] = _cacti.result_type_best_Ntwl_get
    if _newclass:best_Ntwl = property(_cacti.result_type_best_Ntwl_get, _cacti.result_type_best_Ntwl_set)
    __swig_setmethods__["best_Ntbl"] = _cacti.result_type_best_Ntbl_set
    __swig_getmethods__["best_Ntbl"] = _cacti.result_type_best_Ntbl_get
    if _newclass:best_Ntbl = property(_cacti.result_type_best_Ntbl_get, _cacti.result_type_best_Ntbl_set)
    __swig_setmethods__["best_Ntspd"] = _cacti.result_type_best_Ntspd_set
    __swig_getmethods__["best_Ntspd"] = _cacti.result_type_best_Ntspd_get
    if _newclass:best_Ntspd = property(_cacti.result_type_best_Ntspd_get, _cacti.result_type_best_Ntspd_set)
    __swig_setmethods__["best_muxover"] = _cacti.result_type_best_muxover_set
    __swig_getmethods__["best_muxover"] = _cacti.result_type_best_muxover_get
    if _newclass:best_muxover = property(_cacti.result_type_best_muxover_get, _cacti.result_type_best_muxover_set)
    __swig_setmethods__["total_routing_power"] = _cacti.result_type_total_routing_power_set
    __swig_getmethods__["total_routing_power"] = _cacti.result_type_total_routing_power_get
    if _newclass:total_routing_power = property(_cacti.result_type_total_routing_power_get, _cacti.result_type_total_routing_power_set)
    __swig_setmethods__["total_power_without_routing"] = _cacti.result_type_total_power_without_routing_set
    __swig_getmethods__["total_power_without_routing"] = _cacti.result_type_total_power_without_routing_get
    if _newclass:total_power_without_routing = property(_cacti.result_type_total_power_without_routing_get, _cacti.result_type_total_power_without_routing_set)
    __swig_setmethods__["total_power_allbanks"] = _cacti.result_type_total_power_allbanks_set
    __swig_getmethods__["total_power_allbanks"] = _cacti.result_type_total_power_allbanks_get
    if _newclass:total_power_allbanks = property(_cacti.result_type_total_power_allbanks_get, _cacti.result_type_total_power_allbanks_set)
    __swig_setmethods__["subbank_address_routing_delay"] = _cacti.result_type_subbank_address_routing_delay_set
    __swig_getmethods__["subbank_address_routing_delay"] = _cacti.result_type_subbank_address_routing_delay_get
    if _newclass:subbank_address_routing_delay = property(_cacti.result_type_subbank_address_routing_delay_get, _cacti.result_type_subbank_address_routing_delay_set)
    __swig_setmethods__["subbank_address_routing_power"] = _cacti.result_type_subbank_address_routing_power_set
    __swig_getmethods__["subbank_address_routing_power"] = _cacti.result_type_subbank_address_routing_power_get
    if _newclass:subbank_address_routing_power = property(_cacti.result_type_subbank_address_routing_power_get, _cacti.result_type_subbank_address_routing_power_set)
    __swig_setmethods__["decoder_delay_data"] = _cacti.result_type_decoder_delay_data_set
    __swig_getmethods__["decoder_delay_data"] = _cacti.result_type_decoder_delay_data_get
    if _newclass:decoder_delay_data = property(_cacti.result_type_decoder_delay_data_get, _cacti.result_type_decoder_delay_data_set)
    __swig_setmethods__["decoder_delay_tag"] = _cacti.result_type_decoder_delay_tag_set
    __swig_getmethods__["decoder_delay_tag"] = _cacti.result_type_decoder_delay_tag_get
    if _newclass:decoder_delay_tag = property(_cacti.result_type_decoder_delay_tag_get, _cacti.result_type_decoder_delay_tag_set)
    __swig_setmethods__["decoder_power_data"] = _cacti.result_type_decoder_power_data_set
    __swig_getmethods__["decoder_power_data"] = _cacti.result_type_decoder_power_data_get
    if _newclass:decoder_power_data = property(_cacti.result_type_decoder_power_data_get, _cacti.result_type_decoder_power_data_set)
    __swig_setmethods__["decoder_power_tag"] = _cacti.result_type_decoder_power_tag_set
    __swig_getmethods__["decoder_power_tag"] = _cacti.result_type_decoder_power_tag_get
    if _newclass:decoder_power_tag = property(_cacti.result_type_decoder_power_tag_get, _cacti.result_type_decoder_power_tag_set)
    __swig_setmethods__["dec_data_driver"] = _cacti.result_type_dec_data_driver_set
    __swig_getmethods__["dec_data_driver"] = _cacti.result_type_dec_data_driver_get
    if _newclass:dec_data_driver = property(_cacti.result_type_dec_data_driver_get, _cacti.result_type_dec_data_driver_set)
    __swig_setmethods__["dec_data_3to8"] = _cacti.result_type_dec_data_3to8_set
    __swig_getmethods__["dec_data_3to8"] = _cacti.result_type_dec_data_3to8_get
    if _newclass:dec_data_3to8 = property(_cacti.result_type_dec_data_3to8_get, _cacti.result_type_dec_data_3to8_set)
    __swig_setmethods__["dec_data_inv"] = _cacti.result_type_dec_data_inv_set
    __swig_getmethods__["dec_data_inv"] = _cacti.result_type_dec_data_inv_get
    if _newclass:dec_data_inv = property(_cacti.result_type_dec_data_inv_get, _cacti.result_type_dec_data_inv_set)
    __swig_setmethods__["dec_tag_driver"] = _cacti.result_type_dec_tag_driver_set
    __swig_getmethods__["dec_tag_driver"] = _cacti.result_type_dec_tag_driver_get
    if _newclass:dec_tag_driver = property(_cacti.result_type_dec_tag_driver_get, _cacti.result_type_dec_tag_driver_set)
    __swig_setmethods__["dec_tag_3to8"] = _cacti.result_type_dec_tag_3to8_set
    __swig_getmethods__["dec_tag_3to8"] = _cacti.result_type_dec_tag_3to8_get
    if _newclass:dec_tag_3to8 = property(_cacti.result_type_dec_tag_3to8_get, _cacti.result_type_dec_tag_3to8_set)
    __swig_setmethods__["dec_tag_inv"] = _cacti.result_type_dec_tag_inv_set
    __swig_getmethods__["dec_tag_inv"] = _cacti.result_type_dec_tag_inv_get
    if _newclass:dec_tag_inv = property(_cacti.result_type_dec_tag_inv_get, _cacti.result_type_dec_tag_inv_set)
    __swig_setmethods__["wordline_delay_data"] = _cacti.result_type_wordline_delay_data_set
    __swig_getmethods__["wordline_delay_data"] = _cacti.result_type_wordline_delay_data_get
    if _newclass:wordline_delay_data = property(_cacti.result_type_wordline_delay_data_get, _cacti.result_type_wordline_delay_data_set)
    __swig_setmethods__["wordline_delay_tag"] = _cacti.result_type_wordline_delay_tag_set
    __swig_getmethods__["wordline_delay_tag"] = _cacti.result_type_wordline_delay_tag_get
    if _newclass:wordline_delay_tag = property(_cacti.result_type_wordline_delay_tag_get, _cacti.result_type_wordline_delay_tag_set)
    __swig_setmethods__["wordline_power_data"] = _cacti.result_type_wordline_power_data_set
    __swig_getmethods__["wordline_power_data"] = _cacti.result_type_wordline_power_data_get
    if _newclass:wordline_power_data = property(_cacti.result_type_wordline_power_data_get, _cacti.result_type_wordline_power_data_set)
    __swig_setmethods__["wordline_power_tag"] = _cacti.result_type_wordline_power_tag_set
    __swig_getmethods__["wordline_power_tag"] = _cacti.result_type_wordline_power_tag_get
    if _newclass:wordline_power_tag = property(_cacti.result_type_wordline_power_tag_get, _cacti.result_type_wordline_power_tag_set)
    __swig_setmethods__["bitline_delay_data"] = _cacti.result_type_bitline_delay_data_set
    __swig_getmethods__["bitline_delay_data"] = _cacti.result_type_bitline_delay_data_get
    if _newclass:bitline_delay_data = property(_cacti.result_type_bitline_delay_data_get, _cacti.result_type_bitline_delay_data_set)
    __swig_setmethods__["bitline_delay_tag"] = _cacti.result_type_bitline_delay_tag_set
    __swig_getmethods__["bitline_delay_tag"] = _cacti.result_type_bitline_delay_tag_get
    if _newclass:bitline_delay_tag = property(_cacti.result_type_bitline_delay_tag_get, _cacti.result_type_bitline_delay_tag_set)
    __swig_setmethods__["bitline_power_data"] = _cacti.result_type_bitline_power_data_set
    __swig_getmethods__["bitline_power_data"] = _cacti.result_type_bitline_power_data_get
    if _newclass:bitline_power_data = property(_cacti.result_type_bitline_power_data_get, _cacti.result_type_bitline_power_data_set)
    __swig_setmethods__["bitline_power_tag"] = _cacti.result_type_bitline_power_tag_set
    __swig_getmethods__["bitline_power_tag"] = _cacti.result_type_bitline_power_tag_get
    if _newclass:bitline_power_tag = property(_cacti.result_type_bitline_power_tag_get, _cacti.result_type_bitline_power_tag_set)
    __swig_setmethods__["sense_amp_delay_data"] = _cacti.result_type_sense_amp_delay_data_set
    __swig_getmethods__["sense_amp_delay_data"] = _cacti.result_type_sense_amp_delay_data_get
    if _newclass:sense_amp_delay_data = property(_cacti.result_type_sense_amp_delay_data_get, _cacti.result_type_sense_amp_delay_data_set)
    __swig_setmethods__["sense_amp_delay_tag"] = _cacti.result_type_sense_amp_delay_tag_set
    __swig_getmethods__["sense_amp_delay_tag"] = _cacti.result_type_sense_amp_delay_tag_get
    if _newclass:sense_amp_delay_tag = property(_cacti.result_type_sense_amp_delay_tag_get, _cacti.result_type_sense_amp_delay_tag_set)
    __swig_setmethods__["sense_amp_power_data"] = _cacti.result_type_sense_amp_power_data_set
    __swig_getmethods__["sense_amp_power_data"] = _cacti.result_type_sense_amp_power_data_get
    if _newclass:sense_amp_power_data = property(_cacti.result_type_sense_amp_power_data_get, _cacti.result_type_sense_amp_power_data_set)
    __swig_setmethods__["sense_amp_power_tag"] = _cacti.result_type_sense_amp_power_tag_set
    __swig_getmethods__["sense_amp_power_tag"] = _cacti.result_type_sense_amp_power_tag_get
    if _newclass:sense_amp_power_tag = property(_cacti.result_type_sense_amp_power_tag_get, _cacti.result_type_sense_amp_power_tag_set)
    __swig_setmethods__["total_out_driver_delay_data"] = _cacti.result_type_total_out_driver_delay_data_set
    __swig_getmethods__["total_out_driver_delay_data"] = _cacti.result_type_total_out_driver_delay_data_get
    if _newclass:total_out_driver_delay_data = property(_cacti.result_type_total_out_driver_delay_data_get, _cacti.result_type_total_out_driver_delay_data_set)
    __swig_setmethods__["total_out_driver_power_data"] = _cacti.result_type_total_out_driver_power_data_set
    __swig_getmethods__["total_out_driver_power_data"] = _cacti.result_type_total_out_driver_power_data_get
    if _newclass:total_out_driver_power_data = property(_cacti.result_type_total_out_driver_power_data_get, _cacti.result_type_total_out_driver_power_data_set)
    __swig_setmethods__["compare_part_delay"] = _cacti.result_type_compare_part_delay_set
    __swig_getmethods__["compare_part_delay"] = _cacti.result_type_compare_part_delay_get
    if _newclass:compare_part_delay = property(_cacti.result_type_compare_part_delay_get, _cacti.result_type_compare_part_delay_set)
    __swig_setmethods__["drive_mux_delay"] = _cacti.result_type_drive_mux_delay_set
    __swig_getmethods__["drive_mux_delay"] = _cacti.result_type_drive_mux_delay_get
    if _newclass:drive_mux_delay = property(_cacti.result_type_drive_mux_delay_get, _cacti.result_type_drive_mux_delay_set)
    __swig_setmethods__["selb_delay"] = _cacti.result_type_selb_delay_set
    __swig_getmethods__["selb_delay"] = _cacti.result_type_selb_delay_get
    if _newclass:selb_delay = property(_cacti.result_type_selb_delay_get, _cacti.result_type_selb_delay_set)
    __swig_setmethods__["compare_part_power"] = _cacti.result_type_compare_part_power_set
    __swig_getmethods__["compare_part_power"] = _cacti.result_type_compare_part_power_get
    if _newclass:compare_part_power = property(_cacti.result_type_compare_part_power_get, _cacti.result_type_compare_part_power_set)
    __swig_setmethods__["drive_mux_power"] = _cacti.result_type_drive_mux_power_set
    __swig_getmethods__["drive_mux_power"] = _cacti.result_type_drive_mux_power_get
    if _newclass:drive_mux_power = property(_cacti.result_type_drive_mux_power_get, _cacti.result_type_drive_mux_power_set)
    __swig_setmethods__["selb_power"] = _cacti.result_type_selb_power_set
    __swig_getmethods__["selb_power"] = _cacti.result_type_selb_power_get
    if _newclass:selb_power = property(_cacti.result_type_selb_power_get, _cacti.result_type_selb_power_set)
    __swig_setmethods__["data_output_delay"] = _cacti.result_type_data_output_delay_set
    __swig_getmethods__["data_output_delay"] = _cacti.result_type_data_output_delay_get
    if _newclass:data_output_delay = property(_cacti.result_type_data_output_delay_get, _cacti.result_type_data_output_delay_set)
    __swig_setmethods__["data_output_power"] = _cacti.result_type_data_output_power_set
    __swig_getmethods__["data_output_power"] = _cacti.result_type_data_output_power_get
    if _newclass:data_output_power = property(_cacti.result_type_data_output_power_get, _cacti.result_type_data_output_power_set)
    __swig_setmethods__["drive_valid_delay"] = _cacti.result_type_drive_valid_delay_set
    __swig_getmethods__["drive_valid_delay"] = _cacti.result_type_drive_valid_delay_get
    if _newclass:drive_valid_delay = property(_cacti.result_type_drive_valid_delay_get, _cacti.result_type_drive_valid_delay_set)
    __swig_setmethods__["drive_valid_power"] = _cacti.result_type_drive_valid_power_set
    __swig_getmethods__["drive_valid_power"] = _cacti.result_type_drive_valid_power_get
    if _newclass:drive_valid_power = property(_cacti.result_type_drive_valid_power_get, _cacti.result_type_drive_valid_power_set)
    __swig_setmethods__["precharge_delay"] = _cacti.result_type_precharge_delay_set
    __swig_getmethods__["precharge_delay"] = _cacti.result_type_precharge_delay_get
    if _newclass:precharge_delay = property(_cacti.result_type_precharge_delay_get, _cacti.result_type_precharge_delay_set)
    __swig_setmethods__["data_nor_inputs"] = _cacti.result_type_data_nor_inputs_set
    __swig_getmethods__["data_nor_inputs"] = _cacti.result_type_data_nor_inputs_get
    if _newclass:data_nor_inputs = property(_cacti.result_type_data_nor_inputs_get, _cacti.result_type_data_nor_inputs_set)
    __swig_setmethods__["tag_nor_inputs"] = _cacti.result_type_tag_nor_inputs_set
    __swig_getmethods__["tag_nor_inputs"] = _cacti.result_type_tag_nor_inputs_get
    if _newclass:tag_nor_inputs = property(_cacti.result_type_tag_nor_inputs_get, _cacti.result_type_tag_nor_inputs_set)
    def __init__(self, *args):
        _swig_setattr(self, result_type, 'this', _cacti.new_result_type(*args))
        _swig_setattr(self, result_type, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_result_type):
        try:
            if self.thisown: destroy(self)
        except: pass


class result_typePtr(result_type):
    def __init__(self, this):
        _swig_setattr(self, result_type, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, result_type, 'thisown', 0)
        _swig_setattr(self, result_type,self.__class__,result_type)
_cacti.result_type_swigregister(result_typePtr)

class total_result_type(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, total_result_type, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, total_result_type, name)
    def __repr__(self):
        return "<%s.%s; proxy of C total_result_type instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["result"] = _cacti.total_result_type_result_set
    __swig_getmethods__["result"] = _cacti.total_result_type_result_get
    if _newclass:result = property(_cacti.total_result_type_result_get, _cacti.total_result_type_result_set)
    __swig_setmethods__["area"] = _cacti.total_result_type_area_set
    __swig_getmethods__["area"] = _cacti.total_result_type_area_get
    if _newclass:area = property(_cacti.total_result_type_area_get, _cacti.total_result_type_area_set)
    __swig_setmethods__["params"] = _cacti.total_result_type_params_set
    __swig_getmethods__["params"] = _cacti.total_result_type_params_get
    if _newclass:params = property(_cacti.total_result_type_params_get, _cacti.total_result_type_params_set)
    def __init__(self, *args):
        _swig_setattr(self, total_result_type, 'this', _cacti.new_total_result_type(*args))
        _swig_setattr(self, total_result_type, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_total_result_type):
        try:
            if self.thisown: destroy(self)
        except: pass


class total_result_typePtr(total_result_type):
    def __init__(self, this):
        _swig_setattr(self, total_result_type, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, total_result_type, 'thisown', 0)
        _swig_setattr(self, total_result_type,self.__class__,total_result_type)
_cacti.total_result_type_swigregister(total_result_typePtr)


cacti_interface = _cacti.cacti_interface
class output_tuples(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, output_tuples, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, output_tuples, name)
    def __repr__(self):
        return "<%s.%s; proxy of C output_tuples instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["delay"] = _cacti.output_tuples_delay_set
    __swig_getmethods__["delay"] = _cacti.output_tuples_delay_get
    if _newclass:delay = property(_cacti.output_tuples_delay_get, _cacti.output_tuples_delay_set)
    __swig_setmethods__["power"] = _cacti.output_tuples_power_set
    __swig_getmethods__["power"] = _cacti.output_tuples_power_get
    if _newclass:power = property(_cacti.output_tuples_power_get, _cacti.output_tuples_power_set)
    def __init__(self, *args):
        _swig_setattr(self, output_tuples, 'this', _cacti.new_output_tuples(*args))
        _swig_setattr(self, output_tuples, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_output_tuples):
        try:
            if self.thisown: destroy(self)
        except: pass


class output_tuplesPtr(output_tuples):
    def __init__(self, this):
        _swig_setattr(self, output_tuples, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, output_tuples, 'thisown', 0)
        _swig_setattr(self, output_tuples,self.__class__,output_tuples)
_cacti.output_tuples_swigregister(output_tuplesPtr)

class cached_tag_entry(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, cached_tag_entry, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, cached_tag_entry, name)
    def __repr__(self):
        return "<%s.%s; proxy of C cached_tag_entry instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["decoder"] = _cacti.cached_tag_entry_decoder_set
    __swig_getmethods__["decoder"] = _cacti.cached_tag_entry_decoder_get
    if _newclass:decoder = property(_cacti.cached_tag_entry_decoder_get, _cacti.cached_tag_entry_decoder_set)
    __swig_setmethods__["wordline"] = _cacti.cached_tag_entry_wordline_set
    __swig_getmethods__["wordline"] = _cacti.cached_tag_entry_wordline_get
    if _newclass:wordline = property(_cacti.cached_tag_entry_wordline_get, _cacti.cached_tag_entry_wordline_set)
    __swig_setmethods__["bitline"] = _cacti.cached_tag_entry_bitline_set
    __swig_getmethods__["bitline"] = _cacti.cached_tag_entry_bitline_get
    if _newclass:bitline = property(_cacti.cached_tag_entry_bitline_get, _cacti.cached_tag_entry_bitline_set)
    __swig_setmethods__["senseamp"] = _cacti.cached_tag_entry_senseamp_set
    __swig_getmethods__["senseamp"] = _cacti.cached_tag_entry_senseamp_get
    if _newclass:senseamp = property(_cacti.cached_tag_entry_senseamp_get, _cacti.cached_tag_entry_senseamp_set)
    __swig_setmethods__["senseamp_outputrisetime"] = _cacti.cached_tag_entry_senseamp_outputrisetime_set
    __swig_getmethods__["senseamp_outputrisetime"] = _cacti.cached_tag_entry_senseamp_outputrisetime_get
    if _newclass:senseamp_outputrisetime = property(_cacti.cached_tag_entry_senseamp_outputrisetime_get, _cacti.cached_tag_entry_senseamp_outputrisetime_set)
    def __init__(self, *args):
        _swig_setattr(self, cached_tag_entry, 'this', _cacti.new_cached_tag_entry(*args))
        _swig_setattr(self, cached_tag_entry, 'thisown', 1)
    def __del__(self, destroy=_cacti.delete_cached_tag_entry):
        try:
            if self.thisown: destroy(self)
        except: pass


class cached_tag_entryPtr(cached_tag_entry):
    def __init__(self, this):
        _swig_setattr(self, cached_tag_entry, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, cached_tag_entry, 'thisown', 0)
        _swig_setattr(self, cached_tag_entry,self.__class__,cached_tag_entry)
_cacti.cached_tag_entry_swigregister(cached_tag_entryPtr)


