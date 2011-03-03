<%!--=========================================

    Frog "About" page.

============================================--%>
<%@inputencoding="ISO-8859-1"%>
<%@pagetemplate=TEMPLATE_index.y%>
<%@pagetemplatearg=title=Write what you want%>
<%@pagetemplatearg=left=About this website%>
<%@session=dontcreate%>
<%

from frog import VERSION
HOMEPAGE = "http://snakelets.sourceforge.net/frog/"

%>
<h3>About this website</h3>
<table cellspacing="10">
<tr><td align="right">Powered by:</td> <td><img src="<%=asset("img/frog.gif")%>" alt="a frog" /> <strong>Frog <%=VERSION%></strong></td> </tr>  
<tr><td /><td><a href="<%=self.escape(HOMEPAGE)%>"><%=self.escape(HOMEPAGE)%></a></td> </tr>  
<tr><td /><td>(Created by Irmen de Jong)</td> </tr>  
<tr><td align="right">Software License:</td><td><a href="http://www.gnu.org/copyleft/gpl.html">GNU GPL</a></td> </tr>  
<tr><td align="right">Running on:</td><td><%=self.Request.getServerSoftware()%></td> </tr>  
<tr><td style="height: 30px"></td></tr>
<tr><td align="right">Website administrator:</td><td><%=self.WebApp.getConfigItem("site-admin-name")%></td> </tr>  
<tr><td /><td><%=self.WebApp.getConfigItem("site-admin-contact")%></td> </tr>  
<tr><td style="height: 30px"></td></tr>
<tr><td align="right">Anti-Spam:</td><td>This blog uses <a href="http://www.google.com/googleblog/2005/01/preventing-comment-spam.html">anti-spam hyperlinks</a> in comments</td> </tr>  
<tr><td /><td><%
if self.WebApp.getConfigItem("antispam"):
    self.write("Comment text checked with:<br />"+str(self.ApplicationCtx.AntiSpam))
%></td> </tr>  
</table>
<br /><br /><br />
<%
back=self.Request.getReferer()
if not back:
    back=self.URLprefix
%>
<p><a href="<%=back%>">&larr; go back</a></p>
