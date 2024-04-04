# Forestimator

## Un brin d'histoire

La cartographie est l'un des outils privilégiés du laboratoire de Gestion des ressources forestières (université de Liège – Faculté de Gembloux Agro-Bio Tech) pour le suivi de vaste peuplement sur de longue période. 
Qu'il s'agisse de l'identification des espèces présentes à partir d'images satellites, de la distinction des stations forestières les plus productives, de celles très peu productives à vocation de refuge pour la faune et la flore, ou encore du suivi de la dernière crise sanitaire, comme celle causée par le scolyte de l'épicéa qui a profité du réchauffement de notre climat, nous utilisons et produisons des résultats sous forme cartographique.

Afin de faciliter la diffusion précise des couches cartographiques, nous avons développé un **portail cartographique** : [Forestimator](https://forestimator.gembloux.ulg.ac.be/). 
Ce geoportail permet la consultation et le téléchargement de nombreuses cartes qui s'étendent sur l'ensemble de la Wallonie (Belgique), à commencer par la carte de composition des essences qui a suscité un énorme intérêt au sein de la communauté des gestionnaires et scientifiques forestiers.

Cependant, afin d'atteindre un public plus vaste, en particulier les plus jeunes, nous avons décliné Forestimator en une version pour téléphone portable. La version Android sera mise en ligne à l'été 2024.

## Pourquoi partager ces codes informatiques?

Les forces vives de notre équipe ne sont pas des informaticiens: nos codes ne sont pas tous à prendre en exemple. 
Cependant, étant donné que notre travail a largement **bénéficié de librairies open source**, il est essentiel de partager les résultats, que ce soit pour exprimer une certaine reconnaissance envers les créateurs des outils que nous utilisons, ou pour inspirer d'autres développeurs qui souhaiteraient avoir un exemple.

GDAL est utilisé de manière intensive pour les traitements cartographiques, tels que le calcul de cartes aux format raster. La librairie [Wt](https://www.webtoolkit.eu/wt) sert pour la création de l'interface web, main dans la main avec [Openlayers](https://openlayers.org/). Flutter et le package [flutter_map](https://pub.dev/packages/flutter_map) sont utilisés pour le développement de l'application Android

contact: JO.Lisein@uliege.be
