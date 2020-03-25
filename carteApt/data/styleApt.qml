<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis version="3.4.11-Madeira" styleCategories="AllStyleCategories" maxScale="0" hasScaleBasedVisibilityFlag="0" minScale="1e+8">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <customproperties>
    <property value="false" key="WMSBackgroundLayer"/>
    <property value="false" key="WMSPublishDataSourceUrl"/>
    <property value="0" key="embeddedWidgets/count"/>
    <property value="Value" key="identify/format"/>
  </customproperties>
  <pipe>
    <rasterrenderer type="paletted" alphaBand="-1" band="1" opacity="1">
      <rasterTransparency/>
      <minMaxOrigin>
        <limits>None</limits>
        <extent>WholeRaster</extent>
        <statAccuracy>Estimated</statAccuracy>
        <cumulativeCutLower>0.02</cumulativeCutLower>
        <cumulativeCutUpper>0.98</cumulativeCutUpper>
        <stdDevFactor>2</stdDevFactor>
      </minMaxOrigin>
      <colorPalette>
        <paletteEntry label="pas de donnée" value="0" alpha="0" color="#a6d93e"/>
        <paletteEntry label="Optimum" value="1" alpha="255" color="#0d4209"/>
        <paletteEntry label="Tolérance" value="2" alpha="255" color="#72ac2f"/>
        <paletteEntry label="Tolérance Elargie" value="3" alpha="255" color="#e5920d"/>
        <paletteEntry label="Exclusion" value="4" alpha="255" color="#d61a10"/>
        <paletteEntry label="Optimum/Tolérance" value="5" alpha="255" color="#0d4209"/>
        <paletteEntry label="Optimum/Tolérance élargie" value="6" alpha="255" color="#0d4209"/>
        <paletteEntry label="Optimum/Exclusion" value="7" alpha="255" color="#0d4209"/>
        <paletteEntry label="Tolérance/Tolérance élargie" value="8" alpha="255" color="#72ac2f"/>
        <paletteEntry label="Tolérance/Exclusion" value="9" alpha="255" color="#72ac2f"/>
		<paletteEntry label="Tolérance élargie/Exclusion" value="10" alpha="255" color="#e5920d"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeRed="255" colorizeOn="0" saturation="0" colorizeStrength="100" grayscaleMode="0" colorizeBlue="128" colorizeGreen="128"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
