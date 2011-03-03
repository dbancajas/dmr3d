<%!--=========================================

 Multi-user 'portal' index page.
 You will see this page when accessing the root url
 and there is no 'rootdiruser' defined.

============================================--%>
<%@inputencoding="ISO-8859-1"%>
<%@pagetemplate=TEMPLATE_index.y%>
<%@pagetemplatearg=title=CACTI 4.1%>
<%@pagetemplatearg=left=%>
<%@session=no%>
<%@import=import os%>
<%@import=import frog.util%>

<%@inherit=frog.SubmitUtils.Submit%>
<%




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
			<td><a href="index.y">Normal Interface</a></td>
			<td>SRAM Size (bytes)</td>
			<td colspan="2"><input name="cache_size" type="text" size="10" value="<%=self.escape(formValues.get("cache_size",""))%>" /> <span class="error"><%=formErrors.get("cache_size","")%></span></td>
		</tr>
		<tr>
			<td><a href="detailed.y">Detailed Interface</a></td>
			<td>Size of each Entry(bytes)</td>
			<td colspan="2"><input name="line_size" type="text" size="10" value="<%=self.escape(formValues.get("line_size",""))%>" /> <span class="error"><%=formErrors.get("line_size","")%></span></td>
		</tr>
		<tr>
			<td><a href="sram.y">SRAM only</a></td>
			<td>Read/Write Ports</td>
			<td colspan="2"><input name="rwports" type="text" size="10" value="<%=self.escape(formValues.get("rwports",""))%>" /> <span class="error"><%=formErrors.get("rwports","")%></span></td>
		</tr>
		<tr>
			<td><a href="faq.y">FAQ</a></td>
			<td>Read Ports</td>
			<td colspan="2"><input name="read_ports" type="text" size="10" value="<%=self.escape(formValues.get("read_ports",""))%>" /> <span class="error"><%=formErrors.get("read_ports","")%></span></td>
		</tr>
		<tr>
			<td></td>
			<td>Write Ports</td>
			<td colspan="2"><input name="write_ports" type="text" size="10" value="<%=self.escape(formValues.get("write_ports",""))%>" /> <span class="error"><%=formErrors.get("write_ports","")%></span></td>
		</tr>
		<tr>
			<td></td>
			<td>Single Ended Read Ports</td>
			<td colspan="2"><input name="ser_ports" type="text" size="10" value="<%=self.escape(formValues.get("ser_ports",""))%>" /> <span class="error"><%=formErrors.get("ser_ports","")%></span></td>
		</tr>
		<tr>
			<td></td>
			<td>Nr. of Bits Read Out</td>
			<td colspan="2"><input name="output" type="text" size="10" value="<%=self.escape(formValues.get("output",""))%>" /> <span class="error"><%=formErrors.get("output","")%></span></td>
		</tr>
		<tr>
			<td></td>
			<td>Technology Node (um)</td>
			<td colspan="2"><input name="technode" type="text" size="10" value="<%=self.escape(formValues.get("technode",""))%>" /> <span class="error"><%=formErrors.get("technode","")%></span></td>
		</tr>
		<tr>
			<td></td>
			<td><input type="hidden" name="action" value="submit" />
				<input type="hidden" name="cacti" value="cache" />
				<input type="hidden" name="pure_sram" value="pure_sram" />
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
		 Number of banks:<%=self.Request.cacti_output.result.subbanks%><br>
		 Total SRAM Size (bytes):<%=self.Request.cacti_output.params.cache_size%><br>
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
		 Cycle Time (ns):<%=self.Request.cacti_output.result.cycle_time*1e9%><br>
		 Total dynamic Read Power at max. freq. (W): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.dynamic/self.Request.cacti_output.result.cycle_time%>
		 <br>
		 Total Read/Write leakage Power all Banks (mW): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.leakage*1e3%><br>
		 Total area subbanked (mm^2): <%=self.Request.cacti_output.area.subbankarea %><br>
	</td>
	 <td>
		 Best Number of Wordline Segments (data): <%=self.Request.cacti_output.result.best_Ndwl%><br>
		 Best Number of Bitline Segments (data): <%=self.Request.cacti_output.result.best_Ndbl%><br>
		 Best Number of Sets per Wordline (data): <%=self.Request.cacti_output.result.best_Nspd%><br>
		 <br>
	 <td>
	</td>
  </tr>
 
 <tr>
 <td>
 <script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(200,300,MWJ_stacked,false,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_delay_data*1e12%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_delay_data*1e12%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_delay_data*1e12%>]);
g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_delay_data*1e12%>]);
g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_delay*1e12%>]);
g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e12%>]);
g.setTitles('','Delay','ps');
g.setXAxis('Data');
g.setYAxis(<%=int(((	self.Request.cacti_output.result.decoder_delay_data+
						self.Request.cacti_output.result.wordline_delay_data+
						self.Request.cacti_output.result.bitline_delay_data+
						self.Request.cacti_output.result.sense_amp_delay_data
					)
				 *1e12 + self.Request.cacti_output.result.data_output_delay*1e12 + 
						 self.Request.cacti_output.result.total_out_driver_delay_data*1e12)*1.2)%>);
g.buildGraph();
</script>
</td>
<td>
<script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
 <script type="text/javascript" language="javascript1.2">
 var g = new MWJ_graph(200,300,MWJ_stacked,true,false,'#ddeeff');
g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_power_data.readOp.dynamic*1e12%>]);
g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_power_data.readOp.dynamic*1e12%>]);
g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_power_data.readOp.dynamic*1e12%>]);
g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_power_data.readOp.dynamic*1e12%>]);
g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e12%>]);
g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e12%>]);
g.setTitles('','Energy per Access','pJ');
g.setXAxis('Data');
g.setYAxis(<%=int(((self.Request.cacti_output.result.decoder_power_data.readOp.dynamic+
					self.Request.cacti_output.result.wordline_power_data.readOp.dynamic+
					self.Request.cacti_output.result.bitline_power_data.readOp.dynamic+
					self.Request.cacti_output.result.sense_amp_power_data.readOp.dynamic
				   )*1e12 + self.Request.cacti_output.result.data_output_power.readOp.dynamic*1e12 + 
				         self.Request.cacti_output.result.total_out_driver_power_data.readOp.dynamic*1e12)*1.2)%>);
g.buildGraph();
</script>
</td>
<td>
<%if (int(self.Request.cacti_output.params.tech_size*1000) != 180) and (int(self.Request.cacti_output.params.tech_size*1000) != 130) and (int(self.Request.cacti_output.params.tech_size*1000) != 100) and (int(self.Request.cacti_output.params.tech_size*1000) != 70) : %>
		Sorry, we don't have leakage numbers for the <%= int(self.Request.cacti_output.params.tech_size*1000) %> nm technology node. We only have leakage numbers for the 180, 130, 100 and 70 nm nodes.
		If somebody could run the BSIM extractions for the other technology nodes, we would be eternally grateful!
		 
	<%else: %>
		<script src="multigraph.js" type="text/javascript" language="javascript1.2"></script>
		 <script type="text/javascript" language="javascript1.2">
		 var g = new MWJ_graph(200,300,MWJ_stacked,false,false,'#ddeeff');
		g.addDataSet('#FF0000','Decode',[<%=self.Request.cacti_output.result.decoder_power_data.readOp.leakage*1e3%>]);
		g.addDataSet('#FF9900','Wordline',[<%=self.Request.cacti_output.result.wordline_power_data.readOp.leakage*1e3%>]);
		g.addDataSet('#66FF99','Bitline',[<%=self.Request.cacti_output.result.bitline_power_data.readOp.leakage*1e3%>]);
		g.addDataSet('#009953','Sense Amp',[<%=self.Request.cacti_output.result.sense_amp_power_data.readOp.leakage*1e3%>]);
		g.addDataSet('#A00933','Data output driver',[<%=self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3%>]);
		g.addDataSet('#509080','Total out driver ',[<%=self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3%>]);
		g.setTitles('','Leakage Power ','mW');
		g.setXAxis('Data');
		g.setYAxis(<%=int(((self.Request.cacti_output.result.decoder_power_data.readOp.leakage+
							self.Request.cacti_output.result.wordline_power_data.readOp.leakage+
							self.Request.cacti_output.result.bitline_power_data.readOp.leakage+
							self.Request.cacti_output.result.sense_amp_power_data.readOp.leakage
						 )*1e3 + self.Request.cacti_output.result.data_output_power.readOp.leakage*1e3 + 
								 self.Request.cacti_output.result.total_out_driver_power_data.readOp.leakage*1e3)*1.2)%>);
		g.buildGraph();
		</script>
	<%end%>
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
		 data output driver (ns):<%=self.Request.cacti_output.result.data_output_delay*1e9%><br>
		 total_out_driver (ns):<%=self.Request.cacti_output.result.total_out_driver_delay_data*1e9%><br>
		 <br>
	</td>
	<td>
		<h5>Power Components:</h5><br>
 
		 Total dynamic Read Energy all Banks (nJ): <%=self.Request.cacti_output.result.total_power_allbanks.readOp.dynamic*1e9%><br>
		 Total dynamic Write Energy all Banks (nJ):<%=self.Request.cacti_output.result.total_power_allbanks.writeOp.dynamic*1e9%><br>
		 Total dynamic Read Energy Without Routing (nJ): <%=self.Request.cacti_output.result.total_power_without_routing.readOp.dynamic*1e9%><br>
		Total dynamic Write Energy Without Routing (nJ): <%=self.Request.cacti_output.result.total_power_without_routing.writeOp.dynamic*1e9%><br>
		Total dynamic Routing Energy (nJ): <%=self.Request.cacti_output.result.total_routing_power.readOp.dynamic*1e9%><br>
		Total Read/Write leakage Power Without Routing (mW): <%=self.Request.cacti_output.result.total_power_without_routing.readOp.leakage*1e3%><br>
		Total Read/Write leakage Routing Power (mW): <%=self.Request.cacti_output.result.total_routing_power.readOp.leakage*1e3%><br>
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
		 <br>
		 Percentage of data ramcells alone of total area: <%=self.Request.cacti_output.area.perc_data %><br>
		 Percentage of total control/routing alone of total area: <%=self.Request.cacti_output.area.perc_cont %><br>
		 <br>
		 Subbank Efficiency : <%=self.Request.cacti_output.area.sub_eff %><br>
		 Total Efficiency : <%=self.Request.cacti_output.area.total_eff %><br>
		 <br>
		 Total area One bank (mm^2): <%=self.Request.cacti_output.area.totalarea %><br>
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
