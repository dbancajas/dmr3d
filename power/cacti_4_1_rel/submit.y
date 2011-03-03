<%!--===========================================
    SUBMIT a NEW ARTICLE,
    or EDIT an EXISTING ARTICLE.
==============================================--%>
<%@inputencoding="ISO-8859-1"%>
<%@pagetemplate=TEMPLATE.y%>
<%@pagetemplatearg=pagetitle=Submit article%>
<%@import=import frog.util%>
<%@inherit=frog.SubmitUtils.Submit%>
<%
import logging
log=logging.getLogger("Snakelets.logger")

log.debug("submit.y")

user=self.SessionCtx.user

self.process(user)      # process the forms

if not self.RequestCtx.formValues:
	smileysOnByDefault=user.smileys
else:
	smileysOnByDefault=None	


edit = self.Request.getArg()=="edit"
if edit:
    if self.Request.getSession().isNew():
        self.abort("Session is invalid. Have (session-)cookies been disabled?")
    entry = self.prepareEditBlogEntry()    # caches it on the session
    if entry and not (hasattr(self.RequestCtx,"previewArticle") and self.RequestCtx.previewArticle):
        # we're editing, and it's not a preview screen, so prefill the form 
        # with the values from the blog entry we've just loaded.
        self.RequestCtx.formValues["title"]=entry.title
        self.RequestCtx.formValues["category"]=entry.category
        self.RequestCtx.formValues["text"]=entry.text
        self.RequestCtx.formValues["text2"]=entry.text2
        self.RequestCtx.formValues["smileys"]=entry.smileys
        self.RequestCtx.formValues["articletype"]=entry.articletype
        self.RequestCtx.formValues["allowcomments"]=entry.allowcomments
    formurl=self.getURL()+"?edit"
    submitvalue = "update-article"
else:
    formurl = self.getURL()+"?new"
    submitvalue = "submit-article"


formErrors = self.RequestCtx.formErrors
formValues = self.RequestCtx.formValues

if not formValues.get("text"):
    formValues["allowcomments"]=True
%>

<%$include="_categoryselectbox.y"%>
<%$include="_showArticle.y"%>

<%!--================== CACTI Output =================--%>
<%if self.Request.cacti_output:%>
 <h4>CACTI version 4 ALPHA </h4><br>
 <h5>Cache Parameters:</h5>
 Number of Subbanks:INSERT STUFF HERE<br>
 Total Cache Size (bytes):<%=self.Request.cacti_output.params.cache_size%><br>
 Size in bytes of Subbank: INSERT STUFF HERE<br>
 Number of sets:<%=self.Request.cacti_output.params.number_of_sets%><br> 
 Associativity:<%=self.Request.cacti_output.params.tag_associativity%><br> 
 Block Size (bytes):<%=self.Request.cacti_output.params.block_size%><br>
 Read/Write Ports:<%=self.Request.cacti_output.params.num_readwrite_ports%><br> 
 Read Ports:<%=self.Request.cacti_output.params.num_read_ports%><br>
 Write Ports:<%=self.Request.cacti_output.params.num_write_ports%><br> 
 Technology Size (nm):<%=int ( self.Request.cacti_output.params.tech_size*1000)%><br> 
 Vdd:<%=self.Request.cacti_output.params.VddPow%><br>
 <br>
 Access Time (ns): <%=self.Request.cacti_output.result.access_time*1e9%><br>
 Data Side (with Output driver) (ns): INSERT STUFF HERE<br>
 Cycle Time (wave pipelined) (ns):<%=self.Request.cacti_output.result.cycle_time*1e9%><br>
 <br>
 Best Number of Wordline Segments (data): <%=self.Request.cacti_output.result.best_Ndwl%><br>
 Best Number of Bitline Segments (data): <%=self.Request.cacti_output.result.best_Ndbl%><br>
 Best Number of Sets per Wordline (data): <%=self.Request.cacti_output.result.best_Nspd%><br>
 <br>
 Best Number of Wordline Segments (tag): <%=self.Request.cacti_output.result.best_Ntwl%><br>
 Best Number of Bitline Segments (tag): <%=self.Request.cacti_output.result.best_Ntbl%><br>
 Best Number of Sets per Wordline (tag): <%=self.Request.cacti_output.result.best_Ntspd%><br>
 <br>
 Time Components:<br>
 decode_data (ns):<%=self.Request.cacti_output.result.decoder_delay_data*1e9%><br>
 wordline data (ns):<%=self.Request.cacti_output.result.wordline_delay_data*1e9%><br>
 bitline data (ns):<%=self.Request.cacti_output.result.bitline_delay_data*1e9%><br>
 sense_amp_data (ns):<%=self.Request.cacti_output.result.sense_amp_delay_data*1e9%><br>
 <br>
 decode_tag (ns):<%=self.Request.cacti_output.result.decoder_delay_tag*1e9%><br>
 wordline_tag (ns):<%=self.Request.cacti_output.result.wordline_delay_tag*1e9%><br>
 bitline_tag (ns):<%=self.Request.cacti_output.result.bitline_delay_tag*1e9%><br>
 sense_amp_tag (ns):<%=self.Request.cacti_output.result.sense_amp_delay_tag*1e9%><br>
 compare (ns):<%=self.Request.cacti_output.result.compare_part_delay*1e9%><br> 
 mux driver (ns):<%=self.Request.cacti_output.result.drive_mux_delay*1e9%><br> 
 sel inverter (ns):<%=self.Request.cacti_output.result.selb_delay*1e9%><br>
 data output driver (ns):<%=self.Request.cacti_output.result.data_output_delay*1e9%><br>
 total_out_driver (ns):<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e9%><br>
 <br>
 <script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(400,300,MWJ_stacked,true,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_delay_data*1e9%>,
								 <%=self.Request.cacti_output.result.decoder_delay_tag*1e9%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_delay_data*1e9%>,
								   <%=self.Request.cacti_output.result.wordline_delay_tag*1e9%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_delay_data*1e9%>,
								  <%=self.Request.cacti_output.result.bitline_delay_tag*1e9%>]);
g.addDataSet('#009933','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_delay_data*1e9%>,
									<%=self.Request.cacti_output.result.sense_amp_delay_tag*1e9%>]);
g.addDataSet('#EE0000','Compare',[0,<%=self.Request.cacti_output.result.compare_part_delay*1e9%>]);
g.addDataSet('#0033FF','Mux Driver',[0,<%=self.Request.cacti_output.result.drive_mux_delay*1e9%>]);
g.addDataSet('#090009','selb',[0,<%=self.Request.cacti_output.result.selb_delay*1e9%>]);
g.setTitles('CACTI 4.0 Alpha','Data and Tag Delay','ns');
g.setXAxis('Data','Tag');
g.setYAxis(<%=max(self.Request.cacti_output.result.decoder_delay_data+
					self.Request.cacti_output.result.wordline_delay_data+
					self.Request.cacti_output.result.bitline_delay_data+
					self.Request.cacti_output.result.sense_amp_delay_data
				 ,
				 self.Request.cacti_output.result.decoder_delay_tag+
				 self.Request.cacti_output.result.wordline_delay_tag+
				 self.Request.cacti_output.result.bitline_delay_tag+
				 self.Request.cacti_output.result.sense_amp_delay_data+
				 self.Request.cacti_output.result.compare_part_delay+
				 self.Request.cacti_output.result.drive_mux_delay+
				 self.Request.cacti_output.result.selb_delay
				 )*1e9 + 0.2%>);
g.buildGraph();
</script>
 
 
 
 
 
 

 Total dynamic Read Power at max. freq. (W): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.dynamic/self.Request.cacti_output.result.cycle_time%><h5></h5><br>
 <br>
 <br>
 <br>
 <br>
 <br>
 <br>
 <br>
 
 <h5>Cycle Time (ns):</h5>
 <hr/> 
<%end%>

<%!--================== PREVIEW ARTICLE =================--%>
<%if hasattr(self.RequestCtx,"previewArticle") and self.RequestCtx.previewArticle:%>
 <h4>Preview of your changes:</h4>
 <%showArticlePreview(self.RequestCtx.previewArticle)%>
 <hr/> 
<%end%>

<%!--================== EDIT/WRITE ARTICLE FORM =================--%>
<%if edit:%><h4>Edit the article written by you on <%=frog.util.mediumdatestr(entry.datetime[0])%>, <%=entry.datetime[1]%></h4>
<h5>There are <%=self.SessionCtx.commentCount%> comments to this article.
<a href="<%=frog.util.articleURL(self,entry)%>">Return to article and comments.</a></h5>
<%else:%><h4>Write a new article.</h4>
<%end%>
<%if formErrors.has_key("_general"):%>
<span class="error">There was an error:</span>
<br /><span class="error"><%=formErrors["_general"]%></span>
<%end%>

<script src="<%=url('blog/articletype.js')%>" type="text/javascript"></script>
<%$include="_editorstart.y"%>

<form action="<%=formurl%>"  method="post" accept-charset="UTF-8">
<table>
  <tr>
	<td>Cache Size</td>
    <td colspan="2"><input name="cache_size" type="text" size="10" value="<%=self.escape(formValues.get("cache_size",""))%>" /> <span class="error"><%=formErrors.get("data_size","")%></span></td>
 </tr>
 <tr>
	<td>Line Size</td>
    <td colspan="2"><input name="line_size" type="text" size="10" value="<%=self.escape(formValues.get("line_size",""))%>" /> <span class="error"><%=formErrors.get("line_size","")%></span></td>
 </tr>
 <tr>
	<td>Associativity</td>
    <td colspan="2"><input name="assoc" type="text" size="10" value="<%=self.escape(formValues.get("assoc",""))%>" /> <span class="error"><%=formErrors.get("assoc","")%></span></td>
 </tr>
 <tr>
	<td>Nr. of Banks</td>
    <td colspan="2"><input name="nrbanks" type="text" size="10" value="<%=self.escape(formValues.get("nrbanks",""))%>" /> <span class="error"><%=formErrors.get("nrbanks","")%></span></td>
 </tr>
 <tr>
	<td>Size</td>
    <td colspan="2"><input name="technode" type="text" size="10" value="<%=self.escape(formValues.get("technode",""))%>" /> <span class="error"><%=formErrors.get("technode","")%></span></td>
 </tr>
	<!-- SELECT ARTICLE TYPE -->
<%
articletype=formValues.get("articletype","normal")
if articletype=="normal":
    r_1_checked='checked="checked"'
    r_2_checked=''
    normal_fields_display=''
    split_fields_display='style="display: none"'
elif articletype=="split":
    r_1_checked=''
    r_2_checked='checked="checked"'
    normal_fields_display='style="display: none"'
    split_fields_display=''

# normal_fields_display=split_fields_display=''    
%>
<tr><td>Article type:</td><td><input id="at_r_1" type="radio" name="articletype" value="normal" <%=r_1_checked%> onclick="return articleType('normal')" />
<label for="at_r_1">normal</label>
<input id="at_r_2" type="radio" name="articletype" value="split" <%=r_2_checked%> onclick="return articleType('split')"/>
<label for="at_r_2">split</label>
</td></tr>
</table>
<table>

<tr>
<%
if formValues.get("allowcomments"):
    checked_allow='checked="checked"'
else:
    checked_allow=''
%>
	  <td>
       <input name="allowcomments" type="checkbox" <%=checked_allow%> id="c_allow" value="true" /> <label for="c_allow">Allow comments?</label>
      </td>
  </tr>
  <tr>
    <td><input type="hidden" name="action" value="submit" />
		<input type="hidden" name="cacti" value="cache" />
      <input name="preview-article" type="submit" value="Preview" />
      <input name="<%=submitvalue%>" type="submit" value="Submit" /> </td>
    <td align="right">
<%if edit:%>
     <input name="delete-article" type="submit" value="Delete it" style="color: red"/>
<%end%>
    <input name="cancel-article" type="submit" value="Cancel changes" /> </td>
  </tr>
</table>
</form>
