<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis styleCategories="AllStyleCategories" hasScaleBasedVisibilityFlag="0" maxScale="0" version="3.4.11-Madeira" minScale="1e+8">
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
    <rasterrenderer opacity="1" band="1" type="paletted" alphaBand="-1">
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
        <paletteEntry color="#040602" label="Tourbières" alpha="255" value="1"/>
        <paletteEntry color="#191ea0" label="Marais" alpha="255" value="2"/>
        <paletteEntry color="#747173" label="Argiles blanches humides" alpha="255" value="3"/>
        <paletteEntry color="#cbcfc2" label="Argiles blanches à RHA" alpha="255" value="4"/>
        <paletteEntry color="#c3a127" label="Plateaux et versants sub-humides, acidoclines" alpha="255" value="5"/>
        <paletteEntry color="#4b91c3" label="Terrasses alluviales humides" alpha="255" value="6"/>
        <paletteEntry color="#25a1d6" label="Terrasses alluviales et fonds de vallees frais" alpha="255" value="7"/>
        <paletteEntry color="#ec242a" label="Stations xero-oligotrophes" alpha="255" value="9"/>
        <paletteEntry color="#e98ee2" label="Versants chauds superficiels, meso-oligotrophes" alpha="255" value="10"/>
        <paletteEntry color="#c754b6" label="Versants chauds, meso-oligotrophes" alpha="255" value="11"/>
        <paletteEntry color="#7b8790" label="Stations oligotrophes des versants frais" alpha="255" value="12"/>
        <paletteEntry color="#319e36" label="Stations hygrosciaphiles acidoclines" alpha="255" value="14"/>
        <paletteEntry color="#d5a5ff" label="Plateaux oligotrophes" alpha="255" value="15"/>
        <paletteEntry color="#4cbc8a" label="Plateaux meso-oligotrophes" alpha="255" value="16"/>
        <paletteEntry color="#8adbfa" label="Plateaux acidoclines" alpha="255" value="17"/>
        <paletteEntry color="#d7ded8" label="Lithosols" alpha="255" value="18"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeStrength="100" saturation="0" colorizeOn="0" colorizeRed="255" colorizeBlue="128" colorizeGreen="128" grayscaleMode="0"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
