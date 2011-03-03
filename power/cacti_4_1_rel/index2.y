<%!--=========================================

 Multi-user 'portal' index page.
 You will see this page when accessing the root url
 and there is no 'rootdiruser' defined.

============================================--%>
<%@inputencoding="ISO-8859-1"%>
<%@pagetemplate=TEMPLATE_index.y%>
<%@pagetemplatearg=title=CACTI 4.0 Beta%>
<%@pagetemplatearg=left=%>
<%@session=no%>
<%@import=import os%>
<%@import=import frog.util%>

<%@inherit=frog.SubmitUtils.Submit%>
<%
#import logging
#log=logging.getLogger("Snakelets.logger")

#log.debug("submit.y")


self.process2()      # process the forms
formErrors = self.RequestCtx.formErrors


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

<form action="<%=formurl%>"  method="post" accept-charset="UTF-8">
<table>

  <tr>
	<td>Cache Size (bytes)</td>
    <td colspan="2"><input name="cache_size" type="text" size="10" value="<%=self.escape(formValues.get("cache_size",""))%>" /> <span class="error"><%=formErrors.get("data_size","")%></span></td>
 </tr>
 <tr>
	<td>Line Size (bytes)</td>
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
	<td>Technology Node (um)</td>
    <td colspan="2"><input name="technode" type="text" size="10" value="<%=self.escape(formValues.get("technode",""))%>" /> <span class="error"><%=formErrors.get("technode","")%></span></td>
 </tr>



<tr>

  </tr>
  <tr>
    <td><input type="hidden" name="action" value="submit" />
		<input type="hidden" name="cacti" value="cache" />
		<input type="hidden" name="simple" value="simple_cache" />
      <input name="preview-article" type="submit" value="Submit" />
    </td>
  </tr>
  </table>
</form>


<%!--================== CACTI Output =================--%>
<%if self.Request.cacti_output:%>
<table>
	 <tr>
		 <td></td>
		 <td><h5>Cache Parameters:</h5></td>
		 <td></td>
	 </tr>
	 <tr>
	 <td>
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
	</td>
	 <td>
		 Access Time (ns): <%=self.Request.cacti_output.result.access_time*1e9%><br>
		 Data Side (with Output driver) (ns): INSERT STUFF HERE<br>
		 Cycle Time (wave pipelined) (ns):<%=self.Request.cacti_output.result.cycle_time*1e9%><br>
		 <br>
	</td>
	 <td>
		 Best Number of Wordline Segments (data): <%=self.Request.cacti_output.result.best_Ndwl%><br>
		 Best Number of Bitline Segments (data): <%=self.Request.cacti_output.result.best_Ndbl%><br>
		 Best Number of Sets per Wordline (data): <%=self.Request.cacti_output.result.best_Nspd%><br>
		 <br>
		 Best Number of Wordline Segments (tag): <%=self.Request.cacti_output.result.best_Ntwl%><br>
		 Best Number of Bitline Segments (tag): <%=self.Request.cacti_output.result.best_Ntbl%><br>
		 Best Number of Sets per Wordline (tag): <%=self.Request.cacti_output.result.best_Ntspd%><br>
		 <br>
	 <td>
	</td>
  </tr>
 
 <tr>
 <td>
 <script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(200,300,MWJ_stacked,false,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_delay_data*1e12%>,
								 <%=self.Request.cacti_output.result.decoder_delay_tag*1e12%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_delay_data*1e12%>,
								   <%=self.Request.cacti_output.result.wordline_delay_tag*1e12%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_delay_data*1e12%>,
								  <%=self.Request.cacti_output.result.bitline_delay_tag*1e12%>]);
g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_delay_data*1e12%>,
									<%=self.Request.cacti_output.result.sense_amp_delay_tag*1e12%>]);
g.addDataSet('#E0E000','Compare',[0,<%=self.Request.cacti_output.result.compare_part_delay*1e12%>]);
g.addDataSet('#0033FF','Mux Driver',[0,<%=self.Request.cacti_output.result.drive_mux_delay*1e12%>]);
g.addDataSet('#090009','selb',[0,<%=self.Request.cacti_output.result.selb_delay*1e12%>]);
g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_delay*1e12%>,
									<%=self.Request.cacti_output.result.data_output_delay*1e12%>]);
g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e12%>,
									<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e12%>]);
g.setTitles('','Data and Tag Delay','ps');
g.setXAxis('Data','Tag');
g.setYAxis(<%=int((max(self.Request.cacti_output.result.decoder_delay_data+
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
				 )*1e12 + self.Request.cacti_output.result.data_output_delay*1e12 + 
						 self.Request.cacti_output.result.total_out_driver_delay_data*1e12)*1.2)%>);
g.buildGraph();
</script>
</td>
<td>
<script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(200,300,MWJ_stacked,true,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_power_data.readOp.dynamic*1e12%>,
								 <%=self.Request.cacti_output.result.decoder_power_tag.readOp.dynamic*1e12%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_power_data.readOp.dynamic*1e12%>,
								   <%=self.Request.cacti_output.result.wordline_power_tag.readOp.dynamic*1e12%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_power_data.readOp.dynamic*1e12%>,
								  <%=self.Request.cacti_output.result.bitline_power_tag.readOp.dynamic*1e12%>]);
g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_power_data.readOp.dynamic*1e12%>,
									<%=self.Request.cacti_output.result.sense_amp_power_tag.readOp.dynamic*1e12%>]);
g.addDataSet('#E0E000','Compare',[0,<%=self.Request.cacti_output.result.compare_part_power.readOp.dynamic*1e12%>]);
g.addDataSet('#0033FF','Mux Driver',[0,<%=self.Request.cacti_output.result.drive_mux_power.readOp.dynamic*1e12%>]);
g.addDataSet('#090009','selb',[0,<%=self.Request.cacti_output.result.selb_power.readOp.dynamic*1e12%>]);
g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e12%>,
									<%=self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e12%>]);
g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e12%>,
									<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e12%>]);
g.setTitles('','Data and Tag Energy per Access','pJ');
g.setXAxis('Data','Tag');
g.setYAxis(<%=int((max(self.Request.cacti_output.result.decoder_power_data.readOp.dynamic+
					self.Request.cacti_output.result.wordline_power_data.readOp.dynamic+
					self.Request.cacti_output.result.bitline_power_data.readOp.dynamic+
					self.Request.cacti_output.result.sense_amp_power_data.readOp.dynamic
				 ,
					self.Request.cacti_output.result.decoder_power_tag.readOp.dynamic+
					self.Request.cacti_output.result.wordline_power_tag.readOp.dynamic+
					self.Request.cacti_output.result.bitline_power_tag.readOp.dynamic+
					self.Request.cacti_output.result.sense_amp_power_tag.readOp.dynamic+
					self.Request.cacti_output.result.compare_part_power.readOp.dynamic+
					#self.Request.cacti_output.result.drive_mux_power.readOp.dynamic+
					self.Request.cacti_output.result.selb_power.readOp.dynamic
				 )*1e12 + self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e12 + 
				         self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e12)*1.2)%>);
g.buildGraph();
</script>
</td>
<td>
<script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(200,300,MWJ_stacked,false,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_power_data.readOp.leakage*1e3%>,
								 <%=self.Request.cacti_output.result.decoder_power_tag.readOp.leakage*1e3%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_power_data.readOp.leakage*1e3%>,
								   <%=self.Request.cacti_output.result.wordline_power_tag.readOp.leakage*1e3%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_power_data.readOp.leakage*1e3%>,
								  <%=self.Request.cacti_output.result.bitline_power_tag.readOp.leakage*1e3%>]);
g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_power_data.readOp.leakage*1e3%>,
									<%=self.Request.cacti_output.result.sense_amp_power_tag.readOp.leakage*1e3%>]);
g.addDataSet('#E0E000','Compare',[0,<%=self.Request.cacti_output.result.compare_part_power.readOp.leakage*1e3%>]);
g.addDataSet('#0033FF','Mux Driver',[0,<%=self.Request.cacti_output.result.drive_mux_power.readOp.leakage*1e3%>]);
g.addDataSet('#090009','selb',[0,<%=self.Request.cacti_output.result.selb_power.readOp.leakage*1e3%>]);
g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3%>,
									<%=self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3%>]);
g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3%>,
									<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3%>]);
g.setTitles('','Data and Tag leakage Power ','mW');
g.setXAxis('Data','Tag');
g.setYAxis(<%=int((max(self.Request.cacti_output.result.decoder_power_data.readOp.leakage+
					self.Request.cacti_output.result.wordline_power_data.readOp.leakage+
					self.Request.cacti_output.result.bitline_power_data.readOp.leakage+
					self.Request.cacti_output.result.sense_amp_power_data.readOp.leakage
				 ,
					self.Request.cacti_output.result.decoder_power_tag.readOp.leakage+
					self.Request.cacti_output.result.wordline_power_tag.readOp.leakage+
					self.Request.cacti_output.result.bitline_power_tag.readOp.leakage+
					self.Request.cacti_output.result.sense_amp_power_tag.readOp.leakage+
					self.Request.cacti_output.result.compare_part_power.readOp.leakage+
					self.Request.cacti_output.result.drive_mux_power.readOp.leakage+
					self.Request.cacti_output.result.selb_power.readOp.leakage
				 )*1e3 + self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3 + 
				         self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3)*1.2)%>);
g.buildGraph();
</script>
</td>

</tr>
 <tr>
	<td>
		 <h5>Time Components:</h5><br>
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
		 <%if self.Request.cacti_output.params.tag_associativity == 1: %>
				valid signal driver (ns):<%=self.Request.cacti_output.result.drive_valid_delay*1e9%><br>
		 <%else:%>
				mux driver (ns):<%=self.Request.cacti_output.result.drive_mux_delay*1e9%><br> 
				sel inverter (ns):<%=self.Request.cacti_output.result.selb_delay*1e9%><br>
		 <%end%> 
		 
		 <br>
		 data output driver (ns):<%=self.Request.cacti_output.result.data_output_delay*1e9%><br>
		 total_out_driver (ns):<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e9%><br>
		 <br>
	</td>
	<td>
		<h5>Power Components:</h5><br>
 
		 <br>
		 Total dynamic Read Power at max. freq. (W): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.dynamic/self.Request.cacti_output.result.cycle_time%><h5></h5><br>
		 Total dynamic Read Energy all Banks (nJ): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.dynamic*1e9%><br>
		 Total leakage Read Power all Banks (mW): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.leakage*1e3%><br>
		 Total dynamic Write Energy all Banks (nJ):<%=self.Request.cacti_output.result.total_power_allbanks.writeOp.dynamic*1e9%><br>
		 Total leakage Read Power all Banks (mW): <%=self.Request.cacti_output.result.total_power_allbanks.writeOp.leakage*1e3%><br>
		 Total dynamic Read Energy Without Routing (nJ): <%=self.Request.cacti_output.result.total_power_without_routing.readOp.dynamic*1e9%><br>
		Total leakage Read Power Without Routing (mW): <%=self.Request.cacti_output.result.total_power_without_routing.readOp.leakage*1e3%><br>
		Total dynamic Write Energy Without Routing (nJ): <%=self.Request.cacti_output.result.total_power_without_routing.writeOp.dynamic*1e9%><br>
		Total leakage Write Power Without Routing (mW): <%=self.Request.cacti_output.result.total_power_without_routing.writeOp.leakage*1e3%><br>
		Total dynamic Routing Energy (nJ): <%=self.Request.cacti_output.result.total_routing_power.readOp.dynamic*1e9%><br>
		Total leakage Routing Power (mW): <%=self.Request.cacti_output.result.total_routing_power.readOp.leakage*1e3%><br>
		<br>
		 decode_data dyn. energy (nJ): <%=self.Request.cacti_output.result.decoder_power_data.readOp.dynamic*1e9%><br>
		 decode_data leak. power (mW): <%=self.Request.cacti_output.result.decoder_power_data.readOp.leakage*1e3%><br>
		 data wordline dyn. energy (nJ): <%=self.Request.cacti_output.result.wordline_power_data.readOp.dynamic*1e9%><br>
		 data wordline leak. power (mW): <%=self.Request.cacti_output.result.wordline_power_data.readOp.leakage*1e3%><br>
		 data bitline read dyn. energy (nJ): <%=self.Request.cacti_output.result.bitline_power_data.readOp.dynamic*1e9%><br>
		 data bitline write dyn. energy (nJ): <%=self.Request.cacti_output.result.bitline_power_data.writeOp.dynamic*1e9%><br>
		 data bitline leak. power (mW): <%=self.Request.cacti_output.result.bitline_power_data.readOp.leakage*1e3%><br>
		 sense_amp_data dyn. energy (nJ): <%=self.Request.cacti_output.result.sense_amp_power_data.readOp.dynamic*1e9%><br>
		 sense_amp_data leak. power (mW): <%=self.Request.cacti_output.result.sense_amp_power_data.readOp.leakage*1e3%><br>
		 <%if self.Request.cacti_output.params.fully_assoc == 0: %>
			something<br>
		 <%else:%>
			decode_tag dyn. energy (nJ): <%=self.Request.cacti_output.result.decoder_power_tag.readOp.dynamic*1e9%><br>
			decode_tag leak. power (mW): <%=self.Request.cacti_output.result.decoder_power_tag.readOp.leakage*1e3%><br>
			tag wordline dyn. energy (nJ): <%=self.Request.cacti_output.result.wordline_power_tag.readOp.dynamic*1e9%><br>
			tag wordline leak. power (mW): <%=self.Request.cacti_output.result.wordline_power_tag.readOp.leakage*1e3%><br>
			tag bitline read dyn. energy (nJ): <%=self.Request.cacti_output.result.bitline_power_tag.readOp.dynamic*1e9%><br>
			tag bitline write dyn. energy (nJ): <%=self.Request.cacti_output.result.bitline_power_tag.writeOp.dynamic*1e9%><br>
			tag bitline leak. power (mW): <%=self.Request.cacti_output.result.bitline_power_tag.readOp.leakage*1e3%><br>
			sense_amp_tag dyn. energy (nJ): <%=self.Request.cacti_output.result.sense_amp_power_tag.readOp.dynamic*1e9%><br>
			sense_amp_tag leak. power (mW): <%=self.Request.cacti_output.result.sense_amp_power_tag.readOp.leakage*1e3%><br>
			compare dyn. energy (nJ): <%=self.Request.cacti_output.result.compare_part_power.readOp.dynamic*1e9%><br>
			compare leak. power (mW): <%=self.Request.cacti_output.result.compare_part_power.readOp.leakage*1e3%><br>
			<%if self.Request.cacti_output.params.tag_associativity == 1: %>
				valid signal driver dyn. energy (nJ): <%=self.Request.cacti_output.result.drive_valid_power.readOp.dynamic*1e9%><br>
				valid signal driver leak. power (mW): <%=self.Request.cacti_output.result.drive_valid_power.readOp.leakage*1e3%><br>
			<%else:%>
				mux driver dyn. energy (nJ): <%=self.Request.cacti_output.result.drive_mux_power.readOp.dynamic*1e9%><br>
				mux driver leak. power (mW): <%=self.Request.cacti_output.result.drive_mux_power.readOp.leakage*1e3%><br>
				sel inverter dyn. energy (nJ): <%=self.Request.cacti_output.result.selb_power.readOp.dynamic*1e9%><br>
				sel inverter leak. power (mW): <%=self.Request.cacti_output.result.selb_power.readOp.leakage*1e3%><br>
			<%end%>
			
		 <%end%>
		 data output dyn. energy (nJ): <%=self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e9%><br>
		 data output leak. power (mW): <%=self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3%><br>
		 total_out_driver dyn. energy (nJ): <%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e9%><br>
		 total_out_driver leak. power (mW): <%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3%><br>
		 <br>
	</td>
	
	<td>
		 <h5>Area Components:</h5><br>
		 Aspect Ratio Total height/width: <%=self.Request.cacti_output.area.aspect_ratio_total%><br>
		 Data array (mm^2): <%=self.Request.cacti_output.area.dataarray_area.scaled_area%><br>
		 Data predecode (mm^2): <%=self.Request.cacti_output.area.datapredecode_area.scaled_area%><br>
		 Data colmux predecode (mm^2): <%=self.Request.cacti_output.area.datacolmuxpredecode_area.scaled_area%><br>
		 Data colmux post decode (mm^2): <%=self.Request.cacti_output.area.datacolmuxpostdecode_area.scaled_area%><br>
		 Data write signal (mm^2): <%=self.Request.cacti_output.area.datawritesig_area.scaled_area%><br>
		 Tag array (mm^2): <%=self.Request.cacti_output.area.tagarray_area.scaled_area%><br>
		 Tag predecode (mm^2): <%=self.Request.cacti_output.area.tagpredecode_area.scaled_area%><br>
		 Tag colmux predecode (mm^2): <%=self.Request.cacti_output.area.tagpredecode_area.scaled_area%><br>
		 Tag colmux post decode (mm^2): <%=self.Request.cacti_output.area.tagcolmuxpostdecode_area.scaled_area%><br>
		 Tag output driver decode (mm^2): <%=self.Request.cacti_output.area.tagoutdrvdecode_area.scaled_area%><br>
		 Tag output driver enable signals (mm^2): <%=self.Request.cacti_output.area.tagoutdrvsig_area.scaled_area%><br>
		 <br>
		 Percentage of data ramcells alone of total area: <%=self.Request.cacti_output.area.perc_data %><br>
		 Percentage of tag ramcells alone of total area: <%=self.Request.cacti_output.area.perc_tag %><br>
		 Percentage of total control/routing alone of total area: <%=self.Request.cacti_output.area.perc_cont %><br>
		 <br>
		 Subbank Efficiency : <%=self.Request.cacti_output.area.sub_eff %><br>
		 Total Efficiency : <%=self.Request.cacti_output.area.total_eff %><br>
		 <br>
		 Total area One Subbank (mm^2): <%=self.Request.cacti_output.area.totalarea %><br>
		 Total area subbanked (mm^2): <%=self.Request.cacti_output.area.subbankarea %><br>
	</td> 
</td>
</table>
 <br>
 <br>
 <br>
 <br>
 <hr/> 
<%end%>

<%!--================== PREVIEW ARTICLE =================--%>
<%if hasattr(self.RequestCtx,"previewArticle") and self.RequestCtx.previewArticle:%>
 <h4>Preview of your changes:</h4>
 <%showArticlePreview(self.RequestCtx.previewArticle)%>
 <hr/> 
<%end%>



<script src="<%=url('blog/articletype.js')%>" type="text/javascript"></script>
<%#$include="blog/_editorstart.y"%>
