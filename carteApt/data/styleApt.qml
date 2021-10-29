<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis styleCategories="AllStyleCategories" minScale="1e+8" hasScaleBasedVisibilityFlag="0" maxScale="0" version="3.4.11-Madeira">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <customproperties>
    <property key="WMSBackgroundLayer" value="false"/>
    <property key="WMSPublishDataSourceUrl" value="false"/>
    <property key="embeddedWidgets/count" value="0"/>
    <property key="identify/format" value="Value"/>
  </customproperties>
  <pipe>
    <rasterrenderer alphaBand="-1" band="1" type="paletted" opacity="1">
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
        <paletteEntry color="#a6d93e" label="pas de donnée" value="0" alpha="0"/>
        <paletteEntry color="#0d4209" label="Optimum" value="1" alpha="255"/>
        <paletteEntry color="#72ac2f" label="Tolérance" value="2" alpha="255"/>
        <paletteEntry color="#e5920d" label="Tolérance Elargie" value="3" alpha="255"/>
        <paletteEntry color="#d61a10" label="Exclusion" value="4" alpha="255"/>
        <paletteEntry color="#72ac2f" label="Optimum/Tolérance" value="5" alpha="255"/>
        <paletteEntry color="#e5920d" label="Optimum/Tolérance élargie" value="6" alpha="255"/>
        <paletteEntry color="#d61a10" label="Optimum/Exclusion" value="7" alpha="255"/>
        <paletteEntry color="#e5920d" label="Tolérance/Tolérance élargie" value="8" alpha="255"/>
        <paletteEntry color="#d61a10" label="Tolérance/Exclusion" value="9" alpha="255"/>
        <paletteEntry color="#d61a10" label="Tolérance élargie/Exclusion" value="10" alpha="255"/>
        <paletteEntry color="#e0e1e0" label="Indéterminé (Zone batie)" value="12" alpha="255"/>
      </colorPalette>
      <colorramp name="[source]" type="randomcolors"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeGreen="128" colorizeRed="255" colorizeStrength="100" grayscaleMode="0" saturation="0" colorizeOn="0" colorizeBlue="128"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
