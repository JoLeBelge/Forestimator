<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis styleCategories="AllStyleCategories" version="3.16.11-Hannover" maxScale="0" hasScaleBasedVisibilityFlag="0" minScale="1e+08">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <temporal mode="0" fetchMode="0" enabled="0">
    <fixedRange>
      <start></start>
      <end></end>
    </fixedRange>
  </temporal>
  <customproperties>
    <property value="false" key="WMSBackgroundLayer"/>
    <property value="false" key="WMSPublishDataSourceUrl"/>
    <property value="0" key="embeddedWidgets/count"/>
  </customproperties>
  <pipe>
    <provider>
      <resampling zoomedOutResamplingMethod="nearestNeighbour" maxOversampling="2" zoomedInResamplingMethod="nearestNeighbour" enabled="false"/>
    </provider>
    <rasterrenderer opacity="1" alphaBand="-1" nodataColor="" type="paletted" band="1">
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
        <paletteEntry color="#00a200" label="Conseillée" alpha="255" value="1"/>
        <paletteEntry color="#9cd891" label="Conseillée, en tenant compte de ses faiblesses" alpha="255" value="2"/>
        <paletteEntry color="#c0f7d7" label="Convient actuellement mais risques climatiques à long terme" alpha="255" value="3"/>
        <paletteEntry color="#f4b183" label="A n'envisager qu'en accompagnement" alpha="255" value="4"/>
        <paletteEntry color="#eefcc8" label="Déconseillée au vu des risques climatiques trop importants" alpha="255" value="5"/>
        <paletteEntry color="#fff06b" label="Très fortement déconseillée au vu des risques climatiques importants" alpha="255" value="6"/>
        <paletteEntry color="#fd6e6b" label="Ne pas envisager" alpha="255" value="7"/>
      </colorPalette>
      <colorramp name="[source]" type="randomcolors"/>
    </rasterrenderer>
    <brightnesscontrast gamma="1" brightness="0" contrast="0"/>
    <huesaturation colorizeBlue="128" colorizeOn="0" colorizeGreen="128" saturation="0" colorizeStrength="100" grayscaleMode="0" colorizeRed="255"/>
    <rasterresampler maxOversampling="2"/>
    <resamplingStage>resamplingFilter</resamplingStage>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
