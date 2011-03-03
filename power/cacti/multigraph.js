/*****************************************************************************************************************
                   MiltiGraph creation script - creates bar graphs and scatter graphs
                       Version 1.0 - written by Mark Wilton-Jones 03-07/09/2003
******************************************************************************************************************

Please see http://www.howtocreate.co.uk/jslibs/ for details and a demo of this script
Please see http://www.howtocreate.co.uk/tutorials/jsexamples/multigraphExample.html for configuration examples
Please see http://www.howtocreate.co.uk/jslibs/termsOfUse.html for terms of use
_________________________________________________________________________

You can put as many graphs on the page as you like

To use:
_________________________________________________________________________

Inbetween the <head> tags, put:

	<script src="PATH TO SCRIPT/multigraph.js" type="text/javascript" language="javascript1.2"></script>
_________________________________________________________________________

To define a graph
width and height are the width and height of the graph area excluding axis markings, labels and legends
For graphType, use one of the variables defined below
Set showLegend to true to show a legend
Set rotateBy90Degrees to true to rotate the axis by 90 degrees clockwise (height and width will not be rotated)
rotateBy90Degrees must always be set to false for scatter graphs
Graph bgColor must be in six figure hexadecimal format - it can be left blank to accept default colouring #bfbfbf

	var g = new MWJ_graph(int width,int height,graphType graphType,bool showLegend,bool rotateBy90Degrees,HTMLcolorString bgColor)

To set the titles for the axis and the graph title (optional - if any is a blank string, it will be ignored)
	g.setTitles(string graphTitle,string XTitle,string YTitle)

To add a set of data (see below for dataSetData format)
	g.addDataSet(6DigitHex color,string label,array dataSetData)

To set column names for all bar charts
	g.setXAxis(string columnName[,etc])

To set X axis stepping (numbers) on scatter graphs
	g.setXAxis(double scaleNumberStepping)

To set Y axis stepping (numbers) on all graphs except relative percentage bar chart
	g.setYAxis(double scaleNumberStepping)

To tell the browser to draw the graph
	g.buildGraph()

dataSetData format
	for bar chart
[double value1,etc.]
	for scatter graph
[[double Xvalue1,double Yvalue1],etc.]

graphType values
MWJ_bar       Bar chart
MWJ_stacked   Stacked bar chart
MWJ_relative  Relative percentage bar chart
MWJ_scatter   Scatter graph

Graphs are created in tables which are assigned the class 'mwjgraph' so you can style them.
_________________________________________________________________________

Eg.
<script type="text/javascript" language="javascript1.2"><!--
var g = new MWJ_graph(400,300,MWJ_stacked,true,false);
g.addDataSet('#000099','Blue title',[10,2,15,37]);
g.addDataSet('#009900','Green title',[51,2,36,22]);
g.addDataSet('#990099','Purple title',[14,4,23,34]);
g.addDataSet('#990000','Red title',[22,46,44,2]);
g.setTitles('Graph Title','X axis title','Y axis title');
g.setXAxis('January','February','March','April');
g.setYAxis(15);
g.buildGraph();
//--></script>

_______________________________________________________________________*/

var MWJ_bar = 0, MWJ_stacked = 1, MWJ_relative = 2, MWJ_scatter = 3, MWJ_graphError = false;
var MWJ_errorMsg = '\n\nno further error messages will be displayed\nthis graph will not be shown';

function MWJ_graph(oW,oH,oType,oLegend,oRotate,oColor) {
	if( typeof( oType ) != 'number' || oType < 0 || oType > 3 ) {
		this.error = 'an invalid graph type was requested'; if( !MWJ_graphError ) { alert( 'MWJ_graph object creation failed\nan invalid graph type was requested\nplease use one of the predefined variables'+MWJ_errorMsg ); } MWJ_graphError = true; }
	if( oType == 3 && oRotate ) {
		this.error = 'scatter graphs cannot be rotated'; if( !MWJ_graphError ) { alert( 'MWJ_graph object creation failed\nscatter graphs cannot be rotated\nplease set the rotate parameter to false'+MWJ_errorMsg ); } MWJ_graphError = true; }
	if( typeof( oW ) != 'number' || typeof( oH ) != 'number' || oW < 10 || oH < 10 || Math.floor(oW) != oW || Math.floor(oH) != oH ) {
		this.error = 'requested graph size was too small'; if( !MWJ_graphError ) { alert( 'MWJ_graph object creation failed\ngraph width and height must be integers >= 10'+MWJ_errorMsg ); } MWJ_graphError = true; }
	this.colours = []; this.labels = []; this.data = [];
	this.width = oW; this.height = oH; this.type = oType; this.legend = oLegend; this.rotate = oRotate; this.bgColor = oColor;
	this.setTitles = function (oGT,oXT,oYT) { this.graphTitle = oGT; this.XTitle = oXT; this.YTitle = oYT; }
	this.addDataSet = function (oCol,oLab,oData) {
		if( this.error ) { return; }
		if( typeof( oCol ) != 'string' || oCol.length != 7 || oCol.charAt(0) != '#' ) {
			this.error = 'an invalid hex colour was used'; if( !MWJ_graphError ) { alert( 'addDataSet instruction failed\nexpected colour in format #rrggbb'+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		this.colours[this.colours.length] = oCol;
		this.labels[this.labels.length] = oLab;
		if( ( this.type != 3 && this.data[0] && this.data[0].length != oData.length ) || ( this.type != 3 && this.XAxis && this.XAxis.length != oData.length ) ) {
			this.error = 'the number of data columns did not match previously used value'; if( !MWJ_graphError ) { alert( 'addDataSet instruction failed\nexpected previously used number of data columns'+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		if( typeof( oData ) == 'string' || !oData.length ) {
			this.error = 'an invalid dataset was provided'; if( !MWJ_graphError ) { alert( 'addDataSet instruction failed\nexpected an array of data'+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		for( var x = 0; x < oData.length; x++ ) {
			if( ( this.type != 3 && typeof( oData[x] ) != 'number' ) || ( this.type == 3 && ( typeof( oData[x] ) == 'string' || oData[x].length != 2 || typeof( oData[x][0] ) != 'number' || typeof( oData[x][1] ) != 'number' ) ) ) {
				this.error = 'an invalid dataset was provided'; if( !MWJ_graphError ) { alert( 'addDataSet instruction failed because data format was invalid\nexpected an array of '+((this.type==3)?'arrays, each of which must contain 2 numbers':'numbers')+MWJ_errorMsg ); } MWJ_graphError = true; return; }
			if( ( this.type == 1 || this.type == 2 ) && oData[x] < 0 ) {
				this.error = 'data values less that 0 cannot be accepted for this type of graph'; if( !MWJ_graphError ) { alert( 'addDataSet instruction failed because data was invalid\nexpected data values greter than or equal to 0'+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		}
		this.data[this.data.length] = oData;
	};
	this.setXAxis = function () {
		if( this.error ) { return; }
		if( ( this.type != 3 && !arguments.length ) || ( this.type == 3 && ( arguments[0] <= 0 || arguments.length != 1 || typeof(arguments[0]) != 'number' ) ) ) {
			this.error = 'setXAxis was incorrectly used'; if( !MWJ_graphError ) { alert( 'setXAxis instruction failed\nexpected '+((this.type==3)?'a positive number':'list of column titles')+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		if( this.type != 3 && this.data[0] && this.data[0].length != arguments.length ) {
			this.error = 'the number of data columns did not match previously used value'; if( !MWJ_graphError ) { alert( 'setXAxis instruction failed\nexpected previously used number of data columns'+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		if( this.type == 3 ) { this.XAxis = arguments[0]; } else {
			this.XAxis = []; for( var x = 0; x < arguments.length; x++ ) { this.XAxis[x] = arguments[x]; } }
	}
	this.setYAxis = function (oStep) {
		if( this.error ) { return; }
		if( typeof(oStep) != 'number' || this.type == 2 || oStep <= 0 ) {
			this.error = 'setYAxis was incorrectly used'; if( !MWJ_graphError ) { alert( 'setYAxis instruction failed\n'+((this.type==2)?'this method does not apply to relative percentage bar charts':'a positive number')+MWJ_errorMsg ); } MWJ_graphError = true; return; }
		this.YAxis = oStep;
	}
	this.buildGraph = function () {
		if( !this.error && !this.data.length ) {
			this.error = 'no data was provided'; if( !MWJ_graphError ) { alert( 'Graph drawing failed\nno data sets were provided so there is nothing to plot'+MWJ_errorMsg ); } MWJ_graphError = true; }
		if( !this.error && this.data.length < 2 && ( this.type == 1 || this.type == 2 ) ) {
			this.error = 'a minimum of two data sets were expected'; if( !MWJ_graphError ) { alert( 'Graph drawing failed\na minimum of two data sets are required for the chosen graph type'+MWJ_errorMsg ); } MWJ_graphError = true; }
		if( this.error ) { document.write('<div><font color="#ff0000">Graph drawing failed because '+this.error+'.<\/font><\/div>'); return; }
		var leftTitle = this.rotate ? this.XTitle : this.YTitle;
		var botTitle = this.rotate ? this.YTitle : this.XTitle;
		document.write('<table class="mwjgraph" cellpadding="0" cellspacing="7" border="0">');
		var temp = 1 + ( leftTitle ? 1 : 0 ) + ( this.legend ? 1 : 0 );
		if( this.graphTitle ) { document.write('<tr><th'+((temp>1)?(' colspan="'+temp+'"'):'')+(document.layers?'><font size="+1"':' style="font-size:larger;"')+'>'+this.graphTitle+(document.layers?'<\/font>':'')+'<\/th><\/tr>\n'); }
		document.write('<tr>'); var temp = 1 + ( botTitle ? 1 : 0 );
		if( leftTitle && typeof(leftTitle) == 'string' ) {
			document.write('<th'+((temp>1)?(' rowspan="'+temp+'"'):'')+'>');
			for( var x = 0, y = ''; leftTitle.charAt(x); x++ ) { y += leftTitle.charAt(x) + ( leftTitle.charAt(x+1) ? '<br>' : '' ); }
			document.write(y+'<\/th>\n');
		}
		document.write('<td>');
		//end of the chart decoration, start the graph area

		//work out the data ranges
		if( !this.type ) { for( var x = 0, maxH = 0, minH = 0; x < this.data.length; x++ ) { for( var y = 0; y < this.data[x].length; y++ ) {
			if( this.data[x][y] > maxH ) { maxH = this.data[x][y]; } if( this.data[x][y] < minH ) { minH = this.data[x][y]; } } }
		} else if( this.type == 1 ) { for( var y = 0, maxH = 0, minH = 0, totCols = []; y < this.data[0].length; y++ ) { totCols[y] = 0; for( var x = 0; x < this.data.length; x++ ) {
			totCols[y] += this.data[x][y]; } if( totCols[y] > maxH ) { maxH = totCols[y]; } }
		} else if( this.type == 2 ) { for( var y = 0,  maxH = 100, minH = 0, totCols = []; y < this.data[0].length; y++ ) { totCols[y] = 0; for( var x = 0; x < this.data.length; x++ ) {
			totCols[y] += this.data[x][y]; } } this.YAxis = 10;
		} else { for( var x = 0, maxH = 0, minH = 0, maxW = 0, minW = 0; x < this.data.length; x++ ) { for( var y = 0; y < this.data[x].length; y++ ) {
				if( this.data[x][y][1] > maxH ) { maxH = this.data[x][y][1]; } if( this.data[x][y][1] < minH ) { minH = this.data[x][y][1]; }
				if( this.data[x][y][0] > maxW ) { maxW = this.data[x][y][0]; } if( this.data[x][y][0] < minW ) { minW = this.data[x][y][0]; }
			} } if( minW == maxW ) { maxW = 1; }
			if( this.XAxis && maxW % this.XAxis ) { maxW = ( Math.floor( maxW / this.XAxis ) + 1 ) * this.XAxis; }
			if( this.XAxis && minW % this.XAxis ) { minW = ( Math.ceil( minW / this.XAxis ) - 1 ) * this.XAxis; }
			var grWange = maxW - minW;
		}
		if( minH == maxH ) { maxH = 1; }
		if( this.YAxis && maxH % this.YAxis ) { maxH = ( Math.floor( maxH / this.YAxis ) + 1 ) * this.YAxis; }
		if( this.YAxis && minH % this.YAxis ) { minH = ( Math.ceil( minH / this.YAxis ) - 1 ) * this.YAxis; }

		var grRange = (maxH-minH), grSz = (this.rotate?this.width:this.height);

		//work out scale ticks
		var sideTicks = [], botTicks = [];
		if( !this.YAxis ) { if( minH ) { sideTicks[0] = minH; } if( maxH ) { sideTicks[sideTicks.length] = maxH; } } else {
			for( var x = minH; x <= maxH; x += this.YAxis ) { if( x ) { sideTicks[sideTicks.length] = x; } } }
		if( this.type != 3 ) { for( var x = 0; x <= this.data[0].length; x++ ) { botTicks[x] = Math.round( x * ( ( this.rotate ? this.height : this.width ) / this.data[0].length ) ); } } else {
			if( !this.XAxis ) { if( minW ) { botTicks[0] = minW; } botTicks[botTicks.length] = 0; if( maxW ) { botTicks[botTicks.length] = maxW; } } else {
				for( var x = minW; x <= maxW; x += this.XAxis ) { botTicks[botTicks.length] = x; } } }

		document.write('<table cellpadding="0" cellspacing="0" border="0"><tr><td rowspan="2" valign="top" nowrap>'+(document.layers?('<ilayer height="'+this.height+'">'):('<div style="position:relative;left:0px;top:0px;height:'+this.height+'px;">')));
		//start left scale
		if( this.rotate ) { for( var x = 0; this.XAxis && x < this.XAxis.length; x++ ) { if( this.XAxis[x] ) { document.write(MWJ_giveWords(0,( this.height * ( ( ( 2 * x ) + 1 ) / ( 2 * this.XAxis.length ) ) ) - 7,this.XAxis[x])); } } } else {
			for( var x = 0; x < sideTicks.length; x++ ) { var y = (this.height*((maxH-sideTicks[x])/grRange))-7;
				if( sideTicks[x] == minH ) { y -= 5; } if( sideTicks[x] == maxH ) { y += 4; } document.write(MWJ_giveWords(0,y,sideTicks[x]+((this.type==2)?'%':''))); }
			var y = (this.height*(maxH/grRange))-7;
			if( !minH ) { y -= 5; } if( !maxH ) { y += 4; } document.write(MWJ_giveWords(0,y,0+((this.type==2)?'%':'')));
		}
		//end left scale - as for the IE5 Mac fix, don't ask and don't argue, it just works OK!
		var isIE5Mac = document.getElementById && window.ActiveXObject && !navigator.__ice_version && navigator.userAgent.indexOf('Mac') + 1 && window.ScriptEngine && ScriptEngine() == 'JScript';
		document.write((document.layers?'<\/ilayer>':'<\/div>')+'<\/td><td height="'+this.height+'" width="'+this.width+'"'+(isIE5Mac?(' style="position:relative;left:20px;top:0px;" bgcolor="'+(this.bgColor?this.bgColor:'#bfbfbf')+'"><div style="position:absolute;">'):(document.layers?('><ilayer left="0" top="0" width="'+this.width+'" height="'+this.height+'" bgcolor="'+(this.bgColor?this.bgColor:'#bfbfbf')+'">'):('><div style="position:relative;top:0px;left:0px;width:'+this.width+'px;height:'+this.height+'px;background-color:'+(this.bgColor?this.bgColor:'#bfbfbf')+';">'))));
		if( document.layers && !navigator.mimeTypes['*'] ) { document.write('<layer left="0" top="0"><table cellpadding="0" cellspacing="0" border="0"><tr><td height="'+this.height+'" width="'+this.width+'" bgcolor="'+(this.bgColor?this.bgColor:'#bfbfbf')+'">&nbsp;</td></tr></table></layer>'); }
		//start grid
		if( this.type == 3 ) { for( var x = 0; x < botTicks.length; x++ ) { document.write(MWJ_giveBar(10,(this.width*((botTicks[x]-minW)/grWange))-1,(this.height*(maxH/grRange))-5,false)); }
		} else { for( var x = 0; x < botTicks.length; x++ ) { document.write(MWJ_giveBar(grSz,(this.rotate?0:(botTicks[x]-1)),(this.rotate?(botTicks[x]-1):0),this.rotate)); } }
		for( var x = 0; x < sideTicks.length; x++ ) { var y = (this.rotate?(this.width*((sideTicks[x]-minH)/grRange)):(this.height*((maxH-sideTicks[x])/grRange)))-1; document.write(MWJ_giveBar(10,(this.rotate?y:((this.type==3)?(((-1*minW*this.width)/grWange)-5):-5)),(this.rotate?(this.height-5):y),!this.rotate)); }
		var y = (this.rotate?(this.width*((0-minH)/grRange)):(this.height*(maxH/grRange)))-1; document.write(MWJ_giveBar((this.rotate?(this.height+5):(this.width+((this.type==3)?0:5))),(this.rotate?y:((this.type==3)?0:-5)),(this.rotate?0:y),!this.rotate));
		if( this.type == 3 ) { document.write(MWJ_giveBar(10,((-1*minW*this.width)/grWange)-5,(this.height*(maxH/grRange))-1,true));
			document.write(MWJ_giveBar(this.height,((-1*minW*this.width)/grWange)-1,0,false)); }
		//end grid, start bars
		if( !this.type ) {
			for( var x = 0; x < this.data[0].length; x++ ) { for( var y = 0; y < this.data.length; y++ ) { if( this.data[y][x] ) {
				var avlWd = ( botTicks[x+1] - 1 ) - ( botTicks[x] + 1 ); var numExtraPixels = avlWd % this.data.length;
				var scaleSize = Math.round((Math.abs(this.data[y][x])/grRange)*grSz);
				if( this.data[y][x] > 0 ) { var offSet = this.rotate ? Math.round(((0-minH)/grRange)*grSz) : Math.round(((maxH/grRange)*grSz)-scaleSize);
				} else { var offSet = this.rotate ? (Math.round(((0-minH)/grRange)*grSz)-scaleSize) : Math.round((maxH/grRange)*grSz); }
				var offWidth = Math.floor( avlWd / this.data.length ) + ((y<numExtraPixels)?1:0);
				var offSide = botTicks[x] + 1 + ( y * Math.floor( avlWd / this.data.length ) ) + ((y<numExtraPixels)?y:numExtraPixels);
				document.write(MWJ_giveBlock(this.rotate?offSet:offSide,this.rotate?offSide:offSet,this.rotate?scaleSize:offWidth,this.rotate?offWidth:scaleSize,this.colours[y],this.data[y][x]+(this.labels[y]?(' - '+this.labels[y]):'')+((this.XAxis&&this.XAxis[x])?(' - '+this.XAxis[x]):'')));
			} } }
		} else if( this.type == 1 || this.type == 2 ) {
			for( var x = 0; x < this.data[0].length; x++ ) { for( var y = (this.rotate?0:(this.data.length-1)), totData = 0; ( this.rotate && y < this.data.length ) || ( !this.rotate && y >= 0 ); y += (this.rotate?1:-1) ) { if( this.data[y][x] ) {
				var offWidth = ( botTicks[x+1] - 1 ) - ( botTicks[x] + 1 ); var offSide = botTicks[x] + 1; var totDiv = (this.type==1)?maxH:totCols[x];
				if( this.rotate ) { var offSet = Math.round((totData/totDiv)*this.width);
					var scaleSize = Math.round(((totData+this.data[y][x])/totDiv)*this.width) - offSet;
				} else { var offSet = Math.round(((totDiv-(totData+this.data[y][x]))/totDiv)*this.height);
					var scaleSize = Math.round(((totDiv-totData)/totDiv)*this.height) - offSet; }
				totData += this.data[y][x];
				document.write(MWJ_giveBlock(this.rotate?offSet:offSide,this.rotate?offSide:offSet,this.rotate?scaleSize:offWidth,this.rotate?offWidth:scaleSize,this.colours[y],this.data[y][x]+' of '+totCols[x]+((this.type==2)?(' = '+(Math.round((this.data[y][x]/totCols[x])*10000)/100)+'%'):'')+(this.labels[y]?(' - '+this.labels[y]):'')+((this.XAxis&&this.XAxis[x])?(' - '+this.XAxis[x]):'')));
			} } }
		} else { for( var y = 0; y < this.data.length; y++ ) { for( var x = 0; x < this.data[y].length; x++ ) {
			var xPos = Math.round( ( ( this.data[y][x][0] - minW ) * this.width ) / grWange );
			var yPos = Math.round( ( ( maxH - this.data[y][x][1] ) * this.height ) / grRange );
			document.write(MWJ_giveBlock(xPos-2,yPos-2,4,4,this.colours[y],'['+this.data[y][x][0]+','+this.data[y][x][1]+']'+(this.labels[y]?(' - '+this.labels[y]):'')));
		} } }
		//end bars
		document.write((document.layers?'<\/ilayer>':'<\/div>')+'<\/td><\/tr><tr><td valign="top"'+(isIE5Mac?' height="20"':'')+'>'+(document.layers?('<ilayer width="'+this.width+'" height="20">'):('<div style="position:relative;left:0px;top:0px;height:20px;width:'+this.width+'px;">')));
		//start bottom scale
		if( this.rotate ) {
			for( var x = 0; x < sideTicks.length; x++ ) { var y = (this.width*((sideTicks[x]-minH)/grRange))-((''+sideTicks[x]+((this.type==2)?'  ':'')).length*2);
				if( sideTicks[x] == minH ) { y += ((''+sideTicks[x]+'').length*2); } if( sideTicks[x] == maxH ) { y -= ((''+sideTicks[x]+((this.type==2)?'%':'')).length*4); } document.write(MWJ_giveWords(y,5,sideTicks[x]+((this.type==2)?'%':''),true)); }
			var y = (this.width*(0-minH/grRange))-2;
			if( !minH ) { y += 2; } if( !maxH ) { y -= 4; } document.write(MWJ_giveWords(y,5,0+((this.type==2)?'%':''),true));
		} else if( this.type == 3 ) {
			for( var x = 0; x < botTicks.length; x++ ) { var y = (this.width*((botTicks[x]-minW)/grWange))-((''+botTicks[x]+'').length*2);
				if( botTicks[x] == minW ) { y += ((''+botTicks[x]+'').length*2); } if( botTicks[x] == maxW ) { y -= ((''+botTicks[x]+'').length*4); } document.write(MWJ_giveWords(y,5,botTicks[x],true)); }
		} else { for( var x = 0; this.XAxis && x < this.XAxis.length; x++ ) { if( this.XAxis[x] ) { document.write(MWJ_giveWords( ( this.width * ( ( ( 2 * x ) + 1 ) / ( 2 * this.XAxis.length ) ) ) - ((''+this.XAxis[x]+'').length*3),5,this.XAxis[x],true)); } } }
		//end bottom scale - now end graph area
		document.write((document.layers?'<\/ilayer>':'<\/div>')+'<\/td><\/tr><\/table><\/td>');

		//end of the graph area, resume chart decoration
		if( this.legend ) {
			document.write('<td'+((temp>1)?(' rowspan="'+temp+'"'):'')+'><table cellpadding="0" cellspacing="0" border="1"><tr><td>'+
				'<table cellpadding="0" cellspacing="4" border="0"><tr><th colspan="2">Legend<\/th><\/tr>');
			for( var x = 0, y = ''; x < this.data.length; x++ ) { y += '<tr><td bgcolor="'+this.colours[x]+'" width="20">&nbsp;<\/td><td>'+(this.labels[x]?this.labels[x]:'Untitled')+'<\/td><\/tr>'; }
			document.write(y+'<\/table><\/td><\/tr><\/table>\n');
		}
		if( botTitle ) { document.write('<tr><th>'+botTitle+'<\/th><\/tr>'); }
		document.write('<\/table>\n');
	}
}
function MWJ_giveBar(oL,oX,oY,oR) {
	oL = Math.round(oL); oX = Math.round(oX); oY = Math.round(oY);
	if( document.layers ) { return '<layer left="'+oX+'" top="'+oY+'" width="'+(oR?oL:1)+'" height="'+(oR?1:oL)+'" clip="0,0,'+(oR?oL:1)+','+(oR?1:oL)+'" bgcolor="#000000">&nbsp;<\/layer><layer left="'+(oX+(oR?0:1))+'" top="'+(oY+(oR?1:0))+'" width="'+(oR?oL:1)+'" height="'+(oR?1:oL)+'" clip="0,0,'+(oR?oL:1)+','+(oR?1:oL)+'" bgcolor="#ffffff">&nbsp;<\/layer>';
	} else { return '<div style="position:absolute;left:'+oX+'px;top:'+oY+'px;width:'+(oR?oL:1)+'px;height:'+(oR?1:oL)+'px;clip:rect(0px,'+(oR?oL:1)+'px,'+(oR?1:oL)+'px,0px);background-color:#000000;"><\/div><div style="position:absolute;left:'+(oX+(oR?0:1))+'px;top:'+(oY+(oR?1:0))+'px;width:'+(oR?oL:1)+'px;height:'+(oR?1:oL)+'px;clip:rect(0px,'+(oR?oL:1)+'px,'+(oR?1:oL)+'px,0px);background-color:#ffffff;"><\/div>'; }
}
function MWJ_giveWords(oX,oY,oW,oE) {
	oX = Math.round(oX); oY = Math.round(oY);
	if( document.layers ) { return (oE?'':('<ilayer visibility="hide"><font size="-1">'+oW+'&nbsp; &nbsp;<\/font><\/ilayer><br>'))+'<layer left="'+oX+'" top="'+oY+'"><font size="-1">'+oW+'<\/font><\/layer>';
	} else { return (oE?'':('<div style="visibility:hidden;font-size:smaller;">'+oW+'&nbsp; &nbsp;<\/div>'))+'<div style="position:absolute;left:'+oX+'px;top:'+oY+'px;font-size:smaller;">'+oW+'<\/div>'; }
}
function MWJ_giveBlock(oX,oY,oW,oH,oC,oT) {
	if( document.layers ) { return '<layer left="'+oX+'" top="'+oY+'" width="'+oW+'" height="'+oH+'" clip="0,0,'+oW+','+oH+'" bgcolor="'+oC+'" onmouseover="window.status=unescape(\''+escape(oT)+'\');" onmouseout="window.status=window.defaultStatus;">&nbsp;<\/layer>';
	} else { return '<div style="position:absolute;left:'+oX+'px;top:'+oY+'px;width:'+oW+'px;height:'+oH+'px;clip:rect(0px,'+oW+'px,'+oH+'px,0px);background-color:'+oC+';cursor:crosshair;" onclick="alert(this.title);" title="'+oT+'"><\/div>'; }
}

