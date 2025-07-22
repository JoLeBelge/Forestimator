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
        <paletteEntry color="#a6d93e" label="pas de donnée" alpha="0" value="0"/>
        <paletteEntry color="#ded13d" label="Boulaie tourbeuse" alpha="255" value="1"/>
        <paletteEntry color="#060702" label="Aulnaie marécageuse sur substrat mésotrophe" alpha="255" value="2"/>
        <paletteEntry color="#6f1095" label="Aulnaie marécageuse acidophile" alpha="255" value="3"/>
        <paletteEntry color="#7cb255" label="Chênaie pédonculée à bouleau" alpha="255" value="4"/>
        <paletteEntry color="#0c2f06" label="Hêtraie acidophile médio-européenne" alpha="255" value="5"/>
        <paletteEntry color="#abc9ae" label="Chênaie charmaie acidocline médio-européenne" alpha="255" value="6"/>
        <paletteEntry color="#792797" label="Aulnaie-frênaie riveraine des cours d'eau rapides" alpha="255" value="7"/>
        <paletteEntry color="#286017" label="Hêtraie neutrophile médio-européenne à mélique" alpha="255" value="8"/>
        <paletteEntry color="#3b9bda" label="Erablaies-ormaies ardennaises" alpha="255" value="9"/>
        <paletteEntry color="#1a608b" label="Erablaies des coulées pierreuses" alpha="255" value="10"/>
        <paletteEntry color="#50b3ad" label="Chênaie acidophile médio-européenne" alpha="255" value="11"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeStrength="100" saturation="0" colorizeOn="0" colorizeRed="255" colorizeBlue="128" colorizeGreen="128" grayscaleMode="0"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
