#
# Utilities for processing the article submission.
# (the submit page inherits from this class)
#

import frog.objects, frog
import types, time
import cacti
from frog.storageerrors import StorageError\

import pdb


import logging
log=logging.getLogger("Snakelets.logger")

class Submit:
    def prefillFormValues(self,form):
        self.RequestCtx.formValues={}
        for key in form.keys():
            value=form[key]
            if type(value) in types.StringTypes:
                self.RequestCtx.formValues[key]=value.strip()
        self.RequestCtx.formErrors={}

    def cleanup(self):
        if hasattr(self.SessionCtx,"currentEntry"):
            del self.SessionCtx.currentEntry
        if hasattr(self.SessionCtx,"entryToDelete"):
            del self.SessionCtx.entryToDelete

    def process2(self):
        self.Request.setEncoding("UTF-8")
        form=self.Request.getForm()
        self.prefillFormValues(form)
        action=form.get("action")
	is_cacti = form.get("cacti")
	simple = form.get("simple")
	detailed = form.get("detailed")
	pure_sram = form.get("pure_sram")
	if is_cacti:
		cachesize = form.get("cache_size")
		if cachesize == None: 
			self.RequestCtx.formErrors["cache_size"]="Must have a cache size!"
			return
		cachesize = int( form.get("cache_size") )
		if cachesize < 64:
			self.RequestCtx.formErrors["cache_size"]="Cache size must be greater than 32!"
			return
		linesize = form.get("line_size")
		if linesize == None: 
			self.RequestCtx.formErrors["line_size"]="Must have a cacheline size!"
			return
		linesize = int( form.get("line_size") )
		if linesize < 8:
			self.RequestCtx.formErrors["line_size"]="Cache line must be >= 8!"
			return
		technode = form.get("technode")
		if technode == None: 
			self.RequestCtx.formErrors["technode"]="Must have a technology node!"
			return
		technode = float( form.get("technode") )
		if technode > 0.8 or technode <=0 :
			self.RequestCtx.formErrors["technode"]="Feature size must be < 0.8 and > 0!"
			return
		if simple or detailed:
			assoc = form.get("assoc")
			if assoc == None: 
				self.RequestCtx.formErrors["assoc"]="Must have an associativity!"
				return
			int_assoc = int(assoc)
			if int_assoc != 0 and int_assoc != 1 and int_assoc != 2 and int_assoc != 4 and int_assoc != 8 and int_assoc != 16:
				self.RequestCtx.formErrors["assoc"]="Associativity must be  1,2,4,8,16 or 0 (=fully associative)!"
				return
			if (int_assoc != 0) and ((cachesize / (linesize * int_assoc)) < 1):
				self.RequestCtx.formErrors["assoc"]="Number of sets is less than 1!"
				return
			nrbanks = form.get("nrbanks")
			if nrbanks == None: 
				self.RequestCtx.formErrors["nrbanks"]="Must have number of banks!"
				return
			nrbanks = int( form.get("nrbanks") )
			if nrbanks < 1 :
				self.RequestCtx.formErrors["nrbanks"]="Must have a positive number of banks!"
				return

		if detailed or pure_sram:
			rw_ports = form.get("rwports")
			if rw_ports == None: 
				self.RequestCtx.formErrors["rwports"]="Must have a number (zero is okay) of R/W ports!"
				return
			rw_ports = int( form.get("rwports") )
			if rw_ports < 0 :
				self.RequestCtx.formErrors["rwports"]="Number of R/W ports must be non-negative!"
				return
			read_ports = form.get("read_ports")
			if read_ports == None: 
				self.RequestCtx.formErrors["read_ports"]="Must have a number (zero is okay) of read ports!"
				return
			read_ports = int( form.get("read_ports") )
			if read_ports < 0 :
				self.RequestCtx.formErrors["read_ports"]="Number of read ports must be non-negative!"
				return
			read_ports = form.get("read_ports")
			if read_ports == None: 
				self.RequestCtx.formErrors["read_ports"]="Must have a number (zero is okay) of read ports!"
				return
			read_ports = int( form.get("read_ports") )
			if read_ports < 0 :
				self.RequestCtx.formErrors["read_ports"]="Number of read ports must be non-negative!"
				return
			write_ports = form.get("write_ports")
			if write_ports == None: 
				self.RequestCtx.formErrors["write_ports"]="Must have a number (zero is okay) of write ports!"
				return
			write_ports = int( form.get("write_ports") )
			if write_ports < 0 :
				self.RequestCtx.formErrors["write_ports"]="Number of write ports must be non-negative!"
				return
			ser_ports = form.get("ser_ports") 
			if ser_ports == None: 
				self.RequestCtx.formErrors["ser_ports"]="Must have a number (zero is okay) of single ended read ports!"
				return
			ser_ports = int( form.get("ser_ports") )
			if ser_ports < 0 :
				self.RequestCtx.formErrors["ser_ports"]="Number of single ended read ports must be non-negative!"
				return
			bitout = form.get("output") 
			if bitout == None: 
				self.RequestCtx.formErrors["output"]="Must know how many bits to read out!"
				return
			bitout = int( form.get("output") )
			if bitout < 1:
				self.RequestCtx.formErrors["output"]="Output width must be greater than zero!"
				return
		
		if simple:
			output = cacti.cacti_interface(cachesize,linesize, int_assoc,1,0,0,0,nrbanks,technode,64,0,0,0,0)
			self.Request.cacti_output = output
		if detailed:
			specific_tag = form.get("changetag")
			if specific_tag == None: 
				self.RequestCtx.formErrors["changetag"]="You must select!"
				return
			specific_tag = int( form.get("changetag") )
			if(specific_tag):
				tagsize = int( form.get("tagbits") )
			else:
				tagsize = 0
			access_mode = form.get("access_mode") 
			if access_mode == None: 
				self.RequestCtx.formErrors["access_mode"]="You must select an access mode!"
				return
			access_mode = int( form.get("access_mode") )
			output = cacti.cacti_interface(cachesize,linesize,int_assoc,rw_ports,read_ports,write_ports,ser_ports,
			nrbanks,technode,bitout,specific_tag,tagsize,access_mode,0)
			self.Request.cacti_output = output
			
		if pure_sram:
			output = cacti.cacti_interface(cachesize,linesize,1,rw_ports,read_ports,write_ports,ser_ports,1,technode,bitout,0,0,0,1)
			self.Request.cacti_output = output



        #if action and self.Request.getSession().isNew():
        #    self.abort("Session is invalid. Have (session-)cookies been disabled?")

        # check if form submission really is allowed
        #if action:
        #    if not self.User or not self.User.hasPrivilege(frog.USERPRIV_BLOGGER):
        #        self.abort("you're not allowed to submit articles")



    def removeArticle(self, entry, user):
        log.debug("actually deleting article now; id="+str(entry.id))
        entry.remove()
        user.categories[entry.category].count-=1
        user.store()
        self.WebApp.getSnakelet("articlestats.sn").updateStats(self.User.userid)    # update stats for this user
        self.cleanup()
        self.Request.setParameter("status","deleted")
        self.Yredirect("statusmessage.y")

    def prepareEditBlogEntry(self, fromSession=False):
        #if self.User:
        #    log.debug("edit mode; user="+self.User.userid)
        #else:
        #    log.debug("edit mode; not logged in")
        if hasattr(self.SessionCtx,"currentEntry") and (fromSession or self.Request.getParameter("action")):
            log.debug("getting entry from session")
            entry = self.SessionCtx.currentEntry
            date=entry.datetime[0]
            Id=entry.id
        else:
            log.debug("getting date(+id) from req params")
            date = self.Request.getParameter("date")
            Id = int(self.Request.getParameter("id"))
            entry=None
            
        if not entry:
            try:
                log.debug("loading article %s/%d" % (date,Id))
                entry = frog.objects.BlogEntry.load(self.SessionCtx.storageEngine, date, Id)
                self.SessionCtx.currentEntry = entry
                log.debug("loading comment count")
                self.SessionCtx.commentCount = self.SessionCtx.storageEngine.getNumberOfComments(entry.datetime[0], entry.id)
            except StorageError,x:
                self.abort("cannot load article with id "+str(Id))
    
        return entry
