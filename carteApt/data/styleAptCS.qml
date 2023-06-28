<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis hasScaleBasedVisibilityFlag="0" styleCategories="AllStyleCategories" maxScale="0" version="3.10.4-A Coruña" minScale="1e+08">
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
    <rasterrenderer type="paletted" alphaBand="-1" opacity="1" band="1">
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
        <paletteEntry color="#a6d93e" alpha="0" label="pas de donnée" value="0"/>
        <paletteEntry color="#00a200" alpha="255" label="Conseillée sans réserve" value="1"/>
        <paletteEntry color="#c5e0b4" alpha="255" label="Conseillée, en tenant compte de ses faiblesses" value="2"/>
        <paletteEntry color="#f4b183" alpha="255" label="A n'envisager qu'en accompagnement" value="3"/>
        <paletteEntry color="#fd6e6b" alpha="255" label="Ne pas envisager" value="4"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast contrast="0" brightness="0"/>
    <huesaturation grayscaleMode="0" colorizeOn="0" colorizeGreen="128" colorizeStrength="100" colorizeBlue="128" colorizeRed="255" saturation="0"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
