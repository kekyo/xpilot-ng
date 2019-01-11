<?xml version="1.0"?>
<!-- definitions needed for svg objects used -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
               xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
               xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
	       xmlns:xlink="http://www.w3.org/1999/xlink"
>
<!-- XPilotMap should be in every xp2 map -->
<xsl:template match="/XPilotMap">

<!-- write svg header containing width and height from the xp2 map-->
  <xsl:element name="svg">
     <xsl:attribute name="xmlns"><xsl:text>http://www.w3.org/2000/svg</xsl:text></xsl:attribute>
     <xsl:attribute name="sodipodi:version"> <xsl:text>0.32</xsl:text></xsl:attribute>
     <xsl:attribute name="width">
       <xsl:value-of select="/XPilotMap/GeneralOptions/Option[@name='mapwidth']/@value"/>
       <xsl:text> </xsl:text>
     </xsl:attribute>

     <xsl:attribute name="height">
       <xsl:value-of select="/XPilotMap/GeneralOptions/Option[@name='mapheight']/@value"/>
       <xsl:text> </xsl:text>
     </xsl:attribute>

  <xsl:element name="metadata">
  </xsl:element>


    <xsl:call-template name="write-bgcolor"/>
  <!-- define xpilot objects, use clones of these invisible defines for the object lateron-->
  <xsl:element name="defs">
    <xsl:attribute name="id"><xsl:text>defs1</xsl:text></xsl:attribute>
    <xsl:call-template name="write-itemdefs"/>
    <xsl:for-each select="//BmpStyle">
      <xsl:call-template name="BmpStyle"/>
    </xsl:for-each>
    <xsl:for-each select="//Edgestyle">
      <xsl:call-template name="EdgeStyle"/>
    </xsl:for-each>
    <xsl:for-each select="//PolyStyle">
      <xsl:call-template name="PolyStyle"/>
    </xsl:for-each>
  </xsl:element>

 <!-- convert elements of the xp2-file -->

  <xsl:for-each select="Polygon">
    <xsl:call-template name="convert-polygon"/>
  </xsl:for-each>

 <xsl:for-each select="//Fuel">
    <xsl:call-template name="convert-fuel"/>
 </xsl:for-each>

  </xsl:element><!-- end svg -->
</xsl:template><!-- end main-->

<!-- .......................................... -->

<xsl:template name="write-bgcolor">
  <!--xsl:element name="sodipodi:namedview" namespace="sodipodi">
    <xsl:attribute name="pagecolor"><xsl:text>#878787</xsl:text></xsl:attribute>
    <xsl:attribute name="id"><xsl:text>base</xsl:text></xsl:attribute>
    <xsl:attribute name="inkscape:pageopacity" namespace="inkscape"><xsl:text>1.0</xsl:text></xsl:attribute>
  </xsl:element-->
</xsl:template>


<xsl:template name="write-itemdefs">
  <xsl:element name="rect">
    <xsl:attribute name="id">    <xsl:text>fuel</xsl:text></xsl:attribute>
    <xsl:attribute name="width"> <xsl:text>35</xsl:text></xsl:attribute>
    <xsl:attribute name="height"><xsl:text>35</xsl:text></xsl:attribute>
    <xsl:attribute name="style">
      <xsl:text>color:#000000;fill:#ff0000;visibility:visible</xsl:text>
    </xsl:attribute>
  </xsl:element>
</xsl:template>

<xsl:template name="PolyStyle">
<xsl:if test="@texture">
  <xsl:element name="pattern">
    <xsl:attribute name="patternUnits"><xsl:text>userSpaceOnUse</xsl:text></xsl:attribute>
    <xsl:variable name="style"> <xsl:value-of select="@id"/>  </xsl:variable>
    <xsl:attribute name="id">PolyStyle_<xsl:value-of select="@id"/></xsl:attribute>
    <!-- width and height should be width and height of the image for svg -->
    <xsl:attribute name="width">50</xsl:attribute>
    <xsl:attribute name="height">50</xsl:attribute>
     <!--xsl:if test="texture != ''"-->
    <xsl:element name="image">
      <xsl:attribute name="xlink:href">
          <xsl:variable name="texture"> 
             <xsl:value-of select="@texture"/> 
          </xsl:variable>
         <xsl:value-of select="/XPilotMap/BmpStyle[@id=$texture]/@filename" />
      </xsl:attribute>
    <!-- svg should contain height of an image used for a pattern   -->
    <!-- but this information is not part of the current xp2 format -->
    <!-- a value smaller than the image shows a sub-pattern -->
    <!--  50 is normally smaller and gives a rough impression of the texture -->
      <xsl:attribute name="width">50</xsl:attribute>
      <xsl:attribute name="height">50</xsl:attribute>
    </xsl:element>
  </xsl:element>
</xsl:if>
</xsl:template>

<xsl:template name="BmpStyle">
</xsl:template>

<xsl:template name="EdgeStyle">
</xsl:template>


<xsl:template name="convert-polygon">
  <xsl:element name="path">
          <xsl:variable name="style">
             <xsl:value-of select="@style"/>
          </xsl:variable>
          <xsl:variable name="edgestyle">
             <xsl:value-of select="/XPilotMap/PolyStyle[@id=$style]/@defedge"/>
          </xsl:variable>
    <!--xsl:attribute name="tst"><xsl:value-of select="$edgestyle"/></xsl:attribute-->
    <xsl:attribute name="style">fill:url(#PolyStyle_<xsl:value-of select="@style"/>)<!--
    --><xsl:text>;stroke:#</xsl:text><xsl:value-of select="/XPilotMap/EdgeStyle[@id=$edgestyle]/@color" /><!--
    --><xsl:text>;stroke-opacity:1;stroke-width:</xsl:text><!--
    --><xsl:value-of select="/XPilotMap/EdgeStyle[@id=$edgestyle]/@width"/><xsl:text>;<!--
    --></xsl:text></xsl:attribute>
    <xsl:attribute name="d">
      <xsl:text>M </xsl:text>
      <xsl:value-of select="@x*0.015625"/>
      <xsl:text> </xsl:text>
     <xsl:variable name="mapheight">
     <xsl:value-of select="/XPilotMap/GeneralOptions/Option[@name='mapheight']/@value"/>
      </xsl:variable>
      <xsl:value-of select="$mapheight - @y*0.015625"/>
      <xsl:text> </xsl:text>
      <xsl:for-each select="Offset">
        <xsl:text> l </xsl:text>
        <xsl:value-of select="@x*0.015625"/>
        <xsl:text> </xsl:text>
        <xsl:value-of select="@y*-0.015625"/>
      </xsl:for-each>
      <xsl:text> z</xsl:text>
    </xsl:attribute>
  </xsl:element>
</xsl:template>

<xsl:template name="convert-fuel">
  <xsl:element name="use">
    <xsl:attribute name="x"> 
      <xsl:value-of select="@x*0.015625 - 17.5"/>
    </xsl:attribute> 
     <xsl:attribute name="y"> 
       <xsl:variable name="mapheight">
         <xsl:value-of select="/XPilotMap/GeneralOptions/Option[@name='mapheight']/@value"/>
       </xsl:variable>
      <xsl:value-of select="$mapheight - @y*0.015625  - 17.5"/>
    </xsl:attribute> 
   <xsl:attribute name="xlink:href">
     <xsl:text>#fuel</xsl:text> 
   </xsl:attribute>
  </xsl:element>
</xsl:template>


</xsl:stylesheet>

