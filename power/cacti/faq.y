<%!--=========================================

    Frog "About" page.

============================================--%>
<%@inputencoding="ISO-8859-1"%>
<%@pagetemplate=TEMPLATE_index.y%>
<%@pagetemplatearg=title=CACTI 4.1 FAQ%>
<%@pagetemplatearg=left=FAQ%>
<%@session=dontcreate%>
<%

from frog import VERSION
HOMEPAGE = "http://snakelets.sourceforge.net/frog/"

%>
<table cellspacing="10">
<tr><td align="right">Q1:</td> <td> <strong>What do the buttons for Normal/Serial/Fast mean?</strong></td> </tr>  
<tr><td align="right">A1:</td><td>CACTI 4.0 has 3 ways in which a cache access can be organized:<br><br>
             Normal: This is the way standard cache access organization as implemented in CACTI 1.0 to 3.2.<br><br>
			 Serial: With this cache organization the cache tags are accessed first and only the one way which has a match is then read out. A cache organized in
             in this way usually has a longer access time, since it fully serializes the tag and data lookups, but takes less energy per access than a "normal" cache.
			 This organization is employed on some level 2 and 3 which are highly associative and don't lie on the critical path of the processor.<br><br> 
			 Fast: This cache organization  trades of extra energy for a potentially faster access time. Instead of waiting for the way select signal and doing 
			 way selection (a big mux basically) at the data subarray, all N ways are routed to the edge of the data array and only there is way selection done. This means that
			N times more wires are used on the data output path of this organization in comparison to the "normal" organization.</td> </tr> 
<tr><td align="right">Q2:</td> <td> <strong>What does "Change Tag" mean?</strong></td> </tr> 
<tr><td align="right">A2:</td> <td> CACTI 4.0 per default assumes a physical address space of 42 bits and calculates tag lengths accordingly. 
                                           If your processor uses a bigger or smaller address space, than you should change the tag length to what is right for you.
										   The same applies for things like branch target buffers and other structures which don't store cache lines and have their 
										   own rules for tag lengths. </strong></td> </tr> 			
<tr><td /><td>(Created by David Tarjan)</td> </tr>  
</table>
<br /><br /><br />
<%
back=self.Request.getReferer()
if not back:
    back=self.URLprefix
%>
<p><a href="<%=back%>">&larr; go back</a></p>
