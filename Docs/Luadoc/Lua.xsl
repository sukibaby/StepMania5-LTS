<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:sm="http://www.stepmania.com"
	xmlns="http://www.w3.org/1999/xhtml"
	exclude-result-prefixes="sm">
<xsl:output method="xml"
	version="1.0"
	encoding="UTF-8"
	doctype-system="http://www.w3.org/TR/2000/REC-xhtml1-20000126/DTD/xhtml1-strict.dtd"
	doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
	media-type="application/xhtml+xml" />
<!-- Use xsltproc - -stringparam browser no (without a space) to generate the .html. -->
<xsl:param name="browser" select="yes" />
<xsl:template match="/">
<xsl:comment>This file is automatically generated. Do not edit it.</xsl:comment>
	<html>
		<head>
			<title>SM5-LTS API Reference</title>
			<style type="text/css">
			th {
				background: #DDDDDD url(./bgline.png) repeat-x scroll 0 0;
				border: 0px solid #888888;
				border-width: thin;
				padding: 3px;
				font-size: 1.2em;
			}
			thead th {
				background: #EEEEEE url(./bgline.png) repeat-x scroll 0 0;
			}
			table {
				border-style: none;
				border-collapse: collapse;
			}
			td {
				padding: 2px;
				border-style: solid;
				border-width: thin;;
			}
			hr {
				width: 90%;
			}
			code {
				font-family: monospace;
			}
			.code {
				font-family: monospace;
			}
			div fieldset {
				padding: 1px;
			}
			.returnTypeCell {
				font-family: monospace;
				text-align: right;
				vertical-align: text-top;
				width: 10em;
				background: #EEEEEE url(./bgline.png) repeat-x scroll 0 0;
				padding: 1px;
			}
			.descriptionCell {
				text-align: justify;
				vertical-align: text-top;
				background: #DDEEFF url(./bgline.png) repeat-x scroll 0 0;
				padding: 1px;
			}
			fieldset div.descriptionCell{
				margin: 1px 2px 1px 2px;
				border: 1px solid #777;
			}
			.sinceSM5-LTS{
				text-align: justify;
				vertical-align: text-top;
				background: #FFEEDD url(./bgline.png) repeat-x scroll 0 0;
				padding: 1px;
			}
			fieldset div.sinceSM5-LTS{
				margin: 1px 2px 1px 2px;
				border: 1px solid #777;
			}
			.fallbackTheme{
				text-align: justify;
				vertical-align: text-top;
				background: #DDFFEE url(./bgline.png) repeat-x scroll 0 0;
				padding: 1px;
			}
			fieldset div.fallbackTheme{
				margin: 1px 2px 1px 2px;
				border: 1px solid #777;
			}
			.defaultTheme{
				text-align: justify;
				vertical-align: text-top;
				background: #FFD400 url(./bgline.png) repeat-x scroll 0 0;
				padding: 1px;
			}
			fieldset div.defaultTheme{
				margin: 1px 2px 1px 2px;
				border: 1px solid #777;
			}
			.descriptionName {
				font-family: monospace;
				font-weight: bold;
			}
			.descriptionArguments {
				font-family: monospace;
			}
			.descriptionText {
				padding-left: 2em;
				margin-top: 0;
				margin-bottom: 0;
			}
			.primitiveType {
				font-family: monospace;
				color: #0000ff;
			}
			.enumNameCell {
				background-color: #DDEEFF;
				font-size: 1.2em;
			}
			.enumValueCell {
				background-color: #EEEEEE;
				font-size: 1.2em;
			}
			a.classType:link {
				font-family: monospace;
				color: #cc0000;
			}
			a.classType:visited {
				font-family: monospace;
				color: #440000;
			}
			a.enumType:link {
				font-family: monospace;
				color: #cc0000;
			}
			a.enumType:visited {
				font-family: monospace;
				color: #440000;
			}
			.trigger {
				cursor: pointer;
			}
			.footer {
				text-align: center;
			}
			.validate {
				border: 0;
				width: 88px;
				height: 31px;
			}
			#header{
				text-align: center;
				padding-bottom: 0.5em;
			}
			#header h2{
				margin-top: 0px;
				margin-bottom: -1em;
			}
			#sections{
				margin-left: auto;
				margin-right: auto;
			}
			</style>
			<script type="text/javascript">
			function Open( id )
			{
				var imgid = 'img_' + id;
				var listid = 'list_' + id;
				var img = document.getElementById( imgid );
				var list = document.getElementById( listid );

				img.setAttribute( 'src', 'open.gif' );
				list.style.display = 'block';
			}
			function OpenAndMove( classid, functionid )
			{
				Open( classid );
				location.hash = classid + '_' + functionid;
			}
			function OpenOnLoad()
			{
				var hash = window.location.hash;
				if (hash){
					Open( hash.replace("#","") );
				}
			}
			function Toggle( id )
			{
				var imgid = 'img_' + id;
				var listid = 'list_' + id;
				var img = document.getElementById( imgid );
				var list = document.getElementById( listid );

				if( img.getAttribute('src') == 'closed.gif' )
				{
					img.setAttribute( 'src', 'open.gif' );
					list.style.display = 'block';
				}
				else
				{
					img.setAttribute( 'src', 'closed.gif' );
					list.style.display = 'none';
				}
			}
			</script>
			<link rel="icon" type="image/x-icon" href="./favicon.ico"/>
		</head>
		<body>
			<xsl:apply-templates />
			<script>
				OpenOnLoad();
			</script>
		</body>
	</html>
</xsl:template>


<xsl:template match="sm:Lua">
	<div>
		<div id="header">
		<h2><a href="https://github.com/SM5-LTS/SM5-LTS/">SM5-LTS</a> API Reference</h2><br/>
		<small>A collection of Lua bindings which can be used in themes for SM5-LTS.</small>
		</div>
		<table id="sections">
			<thead>
				<tr>
					<th colspan="6">Sections</th>
				</tr>
			</thead>
			<tbody>
			<tr>
				<th><a href="#Singletons">Singletons</a></th>
				<th><a href="#Classes">Classes</a></th>
				<th><a href="#Namespaces">Namespaces</a></th>
				<th><a href="#GlobalFunctions">Global Functions</a></th>
				<th><a href="#Enums">Enums</a></th>
				<th><a href="#Constants">Constants</a></th>
			</tr>
			</tbody>
		</table>
	</div>
	<div>
		<fieldset>
		<legend>Function Colors</legend>
		<div class="sinceSM5-LTS">New in SM5-LTS</div>
		<div class="descriptionCell">Available in StepMania 5</div>
		<div class="fallbackTheme">Defined in the _fallback theme</div>
		<div class="defaultTheme">Defined in the default theme</div>
		</fieldset>
	</div>
	<xsl:apply-templates select="sm:Singletons" />
	<xsl:apply-templates select="sm:Classes" />
	<xsl:apply-templates select="sm:Namespaces" />
	<xsl:apply-templates select="sm:GlobalFunctions" />
	<xsl:apply-templates select="sm:Enums" />
	<xsl:apply-templates select="sm:Constants" />
	<hr />
	<p class="footer">
		Generated for <xsl:value-of select="sm:Version" /> on
		<xsl:value-of select="sm:Date" />.
	</p>
	<p class="footer">
	<xsl:choose>
		<xsl:when test="$browser = 'no'">
			<a href="http://validator.w3.org/check?uri=referer"><img
			class="validate" src="http://www.w3.org/Icons/valid-xhtml10-blue"
			alt="Valid XHTML 1.0 Strict" /></a>
			<a href="http://jigsaw.w3.org/css-validator/check/referer"><img
			class="validate" src="http://www.w3.org/Icons/valid-css2-blue"
			alt="Valid CSS Level 2.1" /></a>
		</xsl:when>
		<xsl:otherwise>
			<a href="http://validator.w3.org/check?uri=referer"><img
			class="validate" src="http://www.w3.org/Icons/valid-xml10-blue"
			alt="Valid XML 1.0" /></a>
		</xsl:otherwise>
	</xsl:choose>
	</p>
</xsl:template>


<xsl:template match="sm:Singletons">
	<div>
		<h3 id="Singletons">Singletons</h3>
		<ul>
			<xsl:for-each select="sm:Singleton">
				<xsl:sort select="@name" />
				<li>
					<a class="classType" href="#{@class}" onclick="Open('{@class}')">
						<xsl:value-of select="@name" />
					</a>
				</li>
			</xsl:for-each>
		</ul>
	</div>
</xsl:template>


<xsl:template match="sm:Classes">
	<div>
		<h3 id="Classes">Classes</h3>
		<xsl:apply-templates select="sm:Class">
			<xsl:sort select="@name" />
		</xsl:apply-templates>
	</div>
</xsl:template>

<xsl:template match="sm:Namespaces">
	<div>
		<h3 id="Namespaces">Namespaces</h3>
		<xsl:apply-templates select="sm:Namespace">
			<xsl:sort select="@name" />
		</xsl:apply-templates>
	</div>
</xsl:template>

<xsl:variable name="docs" select="document('LuaDocumentation.xml')/sm:Documentation" />

<xsl:template match="sm:Class">
	<xsl:variable name="name" select="@name" />
	<div>
		<a id="{@name}" class="trigger" onclick="Toggle('{@name}')">
			<img src="closed.gif" id="img_{@name}" alt="" />
			<xsl:text> Class </xsl:text>
			<span class="descriptionName"><xsl:value-of select="@name" /></span>
		</a>
		<xsl:if test="@base != ''">
			<span class="code"><xsl:text> : </xsl:text></span>
			<a class="classType" href="#{@base}" onclick="Open('{@base}')">
				<xsl:value-of select="@base" />
			</a>
		</xsl:if>
		<div style="display: none" id="list_{@name}">
		<xsl:apply-templates select="$docs/sm:Classes/sm:Class[@name=$name]/sm:Description">
			<xsl:with-param name="class" select="$name" />
		</xsl:apply-templates>
		<table>
			<tr><th colspan="2"><xsl:value-of select="$name" /> Member Functions</th></tr>
			<xsl:apply-templates select="sm:Function">
				<xsl:sort select="@name" />
				<xsl:with-param name="path" select="$docs/sm:Classes/sm:Class[@name=$name]" />
				<xsl:with-param name="class" select="$name" />
			</xsl:apply-templates>
		</table>
		<br />
		</div>
	</div>
</xsl:template>

<xsl:template match="sm:Namespace">
	<xsl:variable name="name" select="@name" />
	<div>
		<a id="{@name}" class="trigger" onclick="Toggle('{@name}')">
			<img src="closed.gif" id="img_{@name}" alt="" />
			<xsl:text> Namespace </xsl:text>
			<span class="descriptionName"><xsl:value-of select="@name" /></span>
		</a>
		<div style="display: none" id="list_{@name}">
		<xsl:apply-templates select="$docs/sm:Namespaces/sm:Namespace[@name=$name]/sm:Description">
			<xsl:with-param name="class" select="$name" />
		</xsl:apply-templates>
		<table>
			<tr><th colspan="2"><xsl:value-of select="$name" /> Functions</th></tr>
			<xsl:apply-templates select="sm:Function">
				<xsl:sort select="@name" />
				<xsl:with-param name="path" select="$docs/sm:Namespaces/sm:Namespace[@name=$name]" />
				<xsl:with-param name="class" select="$name" />
			</xsl:apply-templates>
		</table>
		<br />
		</div>
	</div>
</xsl:template>

<xsl:template match="sm:GlobalFunctions">
	<div>
		<h3 id="GlobalFunctions">Global Functions</h3>
		<table>
			<tr><th colspan="2">Functions</th></tr>
			<xsl:apply-templates select="sm:Function">
				<xsl:sort select="@name" />
				<xsl:with-param name="path" select="$docs/sm:GlobalFunctions" />
				<xsl:with-param name="class" select="'GLOBAL'" />
			</xsl:apply-templates>
		</table>
	</div>
</xsl:template>

<xsl:template name="processType">
	<xsl:param name="type" />
	<xsl:choose>
		<xsl:when test="starts-with($type, '{')">
			<xsl:text>{</xsl:text>
			<xsl:call-template name="processType">
				<xsl:with-param name="type" select="substring-before(
								     substring-after($type, '{'), '}')" />
			</xsl:call-template>
			<xsl:text>}</xsl:text>
		</xsl:when>
		<xsl:when test="$type='void' or
				$type='int' or
				$type='float' or
				$type='string' or
				$type='bool' or
				$type='table' or
				$type='color' or
				$type='Enum' or
				$type='ThreadVariable' or
				$type='LuaOptionRow' or
				$type='ActorDef' or
				$type='chunk' or
				$type='unsigned' or
				$type='various'">
			<span class="primitiveType">
				<xsl:value-of select="$type" />
			</span>
		</xsl:when>
		<xsl:when test="boolean(/sm:Lua/sm:Classes/sm:Class[@name=$type])">
			<a class="classType" href="#{$type}" onclick="Open('{$type}')">
				<xsl:value-of select="$type" />
			</a>
		</xsl:when>
		<xsl:when test="boolean(/sm:Lua/sm:Enums/sm:Enum[@name=$type])">
			<a class="enumType" href="#ENUM_{$type}" onclick="Open('{$type}')">
				<xsl:value-of select="$type" />
			</a>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$type" />
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="processArguments">
	<xsl:param name="argumentList" />
	<xsl:choose>
		<xsl:when test="starts-with($argumentList, ' ')">
			<xsl:call-template name="processArguments">
				<xsl:with-param name="argumentList"
						select="substring-after($argumentList, ' ')" />
			</xsl:call-template>
		</xsl:when>
		<!-- Base cases. -->
		<xsl:when test="$argumentList = '...'">
			<xsl:text>...</xsl:text>
		</xsl:when>
		<xsl:when test="not(contains($argumentList, ','))">
			<xsl:call-template name="processType">
				<xsl:with-param name="type"
				                select="substring-before($argumentList, ' ')" />
			</xsl:call-template>
			<xsl:text> </xsl:text>
			<xsl:value-of select="substring-after($argumentList, ' ')" />
		</xsl:when>
		<xsl:otherwise>
			<xsl:variable name="firstParam"
				      select="substring-before($argumentList, ',')" />
			<xsl:variable name="restParams"
			              select="substring-after($argumentList, ',')" />
			<xsl:call-template name="processType">
				<xsl:with-param name="type"
						select="substring-before($firstParam, ' ')" />
			</xsl:call-template>
			<xsl:text> </xsl:text>
			<xsl:value-of select="substring-after($firstParam, ' ')" /><xsl:text>, </xsl:text>
			<!-- Recursive call. -->
			<xsl:call-template name="processArguments">
				<xsl:with-param name="argumentList" select="$restParams" />
			</xsl:call-template>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="sm:Function">
	<xsl:param name="path" />
	<xsl:param name="class" />
	<xsl:variable name="name" select="@name" />
	<xsl:variable name="elmt" select="$path/sm:Function[@name=$name]" />
	<tr id="{$class}_{$name}">
	<xsl:choose>
		<!-- Check for documentation. -->
		<xsl:when test="string($elmt/@name)=$name">
			<td class="returnTypeCell">
			<xsl:call-template name="processType">
				<xsl:with-param name="type" select="$elmt/@return" />
			</xsl:call-template>
			</td>
			<!-- check for modifiers -->
			<td>
			<xsl:attribute name="class">
				<xsl:choose>
					<xsl:when test="contains($elmt/@since, 'SM5-LTS')">sinceSM5-LTS</xsl:when>
					<xsl:when test="$elmt/@theme='_fallback'">fallbackTheme</xsl:when>
					<xsl:when test="$elmt/@theme='default'">defaultTheme</xsl:when>
					<xsl:otherwise>descriptionCell</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>
			<span class="descriptionName">
				<xsl:value-of select="@name" />
			</span>
			<span class="descriptionArguments">
				<xsl:text>( </xsl:text>
				<xsl:call-template name="processArguments">
					<xsl:with-param name="argumentList"
					                select="$elmt/@arguments" />
				</xsl:call-template>
				<xsl:text> )</xsl:text>
			</span>
			<div class="descriptionText">
				<xsl:apply-templates select="$elmt" mode="print">
					<xsl:with-param name="class" select="$class" />
				</xsl:apply-templates>

				<xsl:if test="$elmt/@since">
					<br />
					<em>New in <xsl:value-of select="$elmt/@since" />.</em>
				</xsl:if>
			</div>
			</td>
		</xsl:when>
		<xsl:otherwise>
			<td class="returnTypeCell" />
			<td>
			<xsl:attribute name="class">
				descriptionCell
			</xsl:attribute>
				<span class="descriptionName"><xsl:value-of select="@name" /></span>
			</td>
		</xsl:otherwise>
	</xsl:choose>
	</tr>
</xsl:template>
<xsl:template match="sm:Function" mode="print">
	<xsl:param name="class" />
	<xsl:apply-templates>
		<xsl:with-param name="curclass" select="$class" />
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="sm:Link">
	<xsl:param name="curclass" />
	<xsl:variable name="empty" select="string(current())=''" />
	<xsl:choose>
		<!-- Linking to a function in the current class/namespace. -->
		<xsl:when test="string(@class)='' and string(@function)!=''">
			<a class="classType" href="#{$curclass}_{@function}">
			<xsl:if test="$empty">
				<xsl:call-template name="sm:PrintLink">
					<xsl:with-param name="class" select="$curclass" />
					<xsl:with-param name="function" select="@function" />
				</xsl:call-template>
			</xsl:if>
			<xsl:apply-templates />
			</a>
		</xsl:when>
		<!-- Linking to a class/namespace. -->
		<xsl:when test="string(@class)!='' and string(@function)=''">
			<a class="classType" href="#{@class}" onclick="Open('{@class}')">
			<xsl:if test="$empty">
				<xsl:value-of select="@class" />
			</xsl:if>
			<xsl:apply-templates />
			</a>
		</xsl:when>
		<!-- Linking to a global function or an enum. -->
		<xsl:when test="(string(@class)='GLOBAL' or string(@class)='ENUM') and string(@function)!=''">
			<a class="classType" href="#{@class}_{@function}" onclick="Open('{@function}')">
			<xsl:if test="$empty">
				<xsl:value-of select="@function" />
				<xsl:if test="string(@class)='GLOBAL'">
					<xsl:text>()</xsl:text>
				</xsl:if>
			</xsl:if>
			<xsl:apply-templates />
			</a>
		</xsl:when>
		<!-- Linking to a function in a class/namespace. -->
		<xsl:when test="string(@class)!='' and string(@function)!=''">
			<a class="classType" href="#{@class}_{@function}" onclick="OpenAndMove('{@class}','{@function}')">
			<xsl:if test="$empty">
				<xsl:call-template name="sm:PrintLink">
					<xsl:with-param name="class" select="@class" />
					<xsl:with-param name="function" select="@function" />
				</xsl:call-template>
			</xsl:if>
			<xsl:apply-templates />
			</a>
		</xsl:when>
		<xsl:otherwise>
			<xsl:apply-templates /> <!-- Ignore this Link. -->
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="sm:PrintLink">
	<xsl:param name="class" />
	<xsl:param name="function" />
	<xsl:value-of select="$class" />
	<xsl:text>.</xsl:text>
	<xsl:value-of select="$function" />
	<xsl:text>()</xsl:text>
</xsl:template>

<xsl:template match="sm:Enums">
	<div>
		<h3 id="Enums">Enums</h3>
		<xsl:apply-templates select="sm:Enum">
			<xsl:sort select="@name" />
		</xsl:apply-templates>
	</div>
</xsl:template>


<xsl:template match="sm:Enum">
	<xsl:variable name="name" select="@name" />
	<div id="ENUM_{@name}">
		<a class="trigger" onclick="Toggle('{@name}')">
		<img src="closed.gif" id="img_{@name}" alt="" />
		<xsl:text> Enum </xsl:text>
		<span class="descriptionName"><xsl:value-of select="@name" /></span></a>
		<div style="display: none" id="list_{@name}">
		<xsl:apply-templates select="$docs/sm:Enums/sm:Enum[@name=$name]/sm:Description">
			<xsl:with-param name="curclass" select="$name" />
		</xsl:apply-templates>
		<table>
			<tr>
				<th>Enum</th>
				<th>Value</th>
			</tr>
			<xsl:for-each select="sm:EnumValue">
				<xsl:sort data-type="number" select="@value" />
				<tr class="code">
					<td class="enumNameCell"><xsl:value-of select="@name" /></td>
					<td class="enumValueCell"><xsl:value-of select="@value" /></td>
				</tr>
			</xsl:for-each>
		</table>
		<br />
		</div>
	</div>
</xsl:template>

<xsl:template match="sm:Description">
	<xsl:param name="class" />
	<p>
		<xsl:apply-templates>
			<xsl:with-param name="curclass" select="$class" />
		</xsl:apply-templates>
	</p>
</xsl:template>

<xsl:template match="sm:Constants">
	<div>
		<h3 id="Constants">Constants</h3>
		<table>
			<tr>
				<th>Constant</th>
				<th>Value</th>
			</tr>
			<xsl:for-each select="sm:Constant">
				<xsl:sort select="@name" />
				<tr class="code">
					<td>

					<xsl:value-of select="@name" />
					</td>
					<td><xsl:value-of select="@value" /></td>
				</tr>
			</xsl:for-each>
		</table>
		<br />
	</div>
</xsl:template>

<!-- XXX: This is annoying, how can we tell xsl to just pass the html through?
     Even more annoying is the fact that parameters aren't dynamically scoped
     so we have to explicitly pass all parameters through <code>. -->
<xsl:template match="sm:code">
	<xsl:param name="curclass" />
	<code>
		<xsl:apply-templates>
			<xsl:with-param name="curclass" select="$curclass" />
		</xsl:apply-templates>
	</code>
</xsl:template>
<xsl:template match="sm:pre">
	<xsl:param name="curclass" />
	<pre>
		<xsl:apply-templates>
			<xsl:with-param name="curclass" select="$curclass" />
		</xsl:apply-templates>
	</pre>
</xsl:template>
<xsl:template match="sm:br"><br /></xsl:template>
<xsl:template match="sm:del">
	<del>
		<xsl:apply-templates>
			<xsl:with-param name="curclass" select="$curclass" />
		</xsl:apply-templates>
	</del>
</xsl:template>
</xsl:stylesheet>
<!-- vim: set tw=0: -->
