#ifndef WBCFFCONSTANTS_H
#define WBCFFCONSTANTS_H

#define PI 3.1415926535

const int DEFAULT_BACKGROUND_LAYER = -20000002;
const int DEFAULT_BACKGROUND_CROSS_LAYER = -20000001;

// Constant fileNames;
const QString fMetadata = "metadata.rdf";
const QString fIWBContent = "content.xml";
const QString fIWBBackground = "background.png";
const QString sAudioElementImage = ":images/soundOn.svg";

// Constant messages;
const QString noErrorMsg = "NoError";

// Tag names
const QString tDescription = "Description";
const QString tIWBRoot = "iwb";
const QString tIWBMeta = "meta";
const QString tWBZSize = "size";
const QString tSvg = "svg";
const QString tIWBPage = "page";
const QString tIWBPageSet = "pageset";
const QString tId = "id";
const QString tElement = "element";
const QString tWBZGroup = "group";
const QString tWBZGroups = "groups";
const QString tUBZG = "g";
const QString tWBZPolygon = "polygon";
const QString tWBZPolyline = "polyline";
const QString tWBZLine = "line";
const QString tWBZAudio = "audio";
const QString tWBZVideo = "video";
const QString tWBZImage = "image";
const QString tWBZForeignObject = "foreignObject";
const QString tWBZTextContent = "itemTextContent";

const QString tIWBA = "a";
const QString tIWBG = "g";
const QString tIWBSwitch = "switch";
const QString tIWBImage = "image";
const QString tIWBVideo = "video";
const QString tIWBAudio = "audio";
const QString tIWBText = "text";
const QString tIWBTextArea = "textarea";
const QString tIWBPolyLine = "polyline";
const QString tIWBPolygon = "polygon";
const QString tIWBFlash = "video";
const QString tIWBRect = "rect";
const QString tIWBLine = "line";
const QString tIWBTbreak = "tbreak";
const QString tIWBTspan = "tspan";

// Attributes names
const QString aIWBVersion = "version";
const QString aOwner  = "owner";
const QString aDescription  = "description";
const QString aCreator  = "creator";
const QString aAbout  = "about";
const QString aIWBViewBox = "viewbox";
const QString aWBZViewBox = "viewBox";
const QString aDarkBackground = "dark-background";
const QString aBackground = "background";
const QString aCrossedBackground = "crossed-background";
const QString aWBZType = "type";
const QString aWBZUuid = "uuid";
const QString aWBZParent = "parent";
const QString aFill = "fill"; 

const QString aID = "id";
const QString aRef = "ref";
const QString aSVGHref = "xlink:href";
const QString aIWBHref = "ref";
const QString aWBZHref = "href";
const QString aWBZSource = "source";
const QString aSrc = "src";
const QString aSVGRequiredExtension = "requiredExtensions";

const QString aX = "x";
const QString aY = "y";
const QString aWidth = "width";
const QString aHeight = "height";
const QString aStroke = "stroke";
const QString aStrokeWidth = "stroke-width";
const QString aPoints = "points";
const QString aZLayer = "z-value";
const QString aLayer = "layer";
const QString aTransform = "transform";
const QString aLocked = "locked";
const QString aIWBName = "name";
const QString aIWBContent = "content";


// Attribute values
const QString avIWBVersionNo = "1.0"; 
const QString avWBZText = "text";
const QString avFalse = "false";
const QString avTrue = "true";

// Namespaces and prefixes
const QString svgWBZNS = "http://www.imsglobal.org/xsd/iwb_v1p0";
const QString svgIWBNS = "http://www.w3.org/2000/svg";
const QString xlinkNS = "http://www.w3.org/1999/xlink";
const QString iwbNS = "http://www.imsglobal.org/xsd/iwb_v1p0";
const QString dcNSPrefix = "dc";
const QString wbNSPrefix = "wb";
const QString svgIWBNSPrefix = "svg";
const QString xlinkNSPrefix = "xlink";
const QString iwbNsPrefix = "iwb";
const QString xsiPrefix = "xsi";
const QString xsiSchemaLocationPrefix = "schemaLocation";

const QString avOwner = "";
const QString avCreator = "";
const QString avDescription = "";

//constant symbols and words etc
const QString dimensionsDelimiter1 = "x";
const QString dimensionsDelimiter2 = " ";
const QString pageAlias = "page";
const QString pageFileExtentionUBZ = "svg";

//content folder names
const QString cfImages = "images";
const QString cfVideos = "video";
const QString cfAudios = "audio";
const QString cfFlash = "flash";

//known file extentions
const QString feSvg = "svg";
const QString feWgt = "wgt";
const QString fePng = "png";

const int iCrossSize = 32;
const int iCrossWidth = 1;

const QString iwbElementImage(" \
wgt, \
jpeg, \
jpg, \
bmp, \
gif, \
wmf, \
emf, \
png, \
tif, \
tiff \
");

const QString iwbElementVideo(" \
mpg, \
mpeg, \
swf, \
");

// Audio formats supported by CFF
const QString iwbElementAudio(" \
mp3, \
wav \
");

const QString cffSupportedFileFormats(iwbElementImage + iwbElementVideo + iwbElementAudio);
const QString wbzFormatsToConvert("svg");

const QString iwbSVGImageAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGAudioAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGVideoAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGRectAttributes(" \
id, \
x, \
y, \
height, \
width, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-linecap, \
stroke-linejoin, \
stroke-opacity, \
stroke-width, \
transform \
");

const QString iwbSVGTextAttributes(" \
id, \
x, \
y, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
transform \
");

const QString iwbSVGTextAreaAttributes(" \
id, \
x, \
y, \
height, \
width, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align, \
transform \
");

const QString iwbSVGTspanAttributes(" \
id, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align, \
");

const QString iwbSVGLineAttributes(" \
id, \
x1, \
y1, \
x2, \
y2, \
stroke, \
stroke-dasharray, \
stroke-width, \
stroke-opacity, \
stroke-linecap, \
transform \
");

const QString iwbSVGPolyLineAttributes(" \
id, \
points, \
stroke, \
stroke-width, \
stroke-dasharray, \
stroke-opacity, \
stroke-linecap, \
transform \
");

const QString iwbSVGPolygonAttributes(" \
id, \
points, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-width, \
stroke-linecap, \
stroke-linejoin, \
stroke-opacity, \
stroke-width, \
transform \
");

const QString iwbElementAttributes(" \
background, \
background-fill, \
background-posture, \
flip, \
freehand, \
highlight, \
highlight-fill, \
list-style-type, \
list-style-type-fill, \
locked, \
replicate, \
revealer, \
stroke-lineshape-start, \
stroke-lineshape-end \
");

const QString wbzElementAttributesToConvert(" \
xlink:href, \
src, \
transform, \
uuid \
"
);

const QString svgElementAttributes(" \
points, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-linecap, \
stroke-opacity, \
stroke-width, \
stroke_linejoin, \
requiredExtensions, \
viewbox, \
x, \
y, \
x1, \
y1, \
x2, \
y2, \
height, \
width, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align \
");

const QString wbzContentFolders("audios,videos,images,widgets");

struct WBItemLayerType
{
    enum Enum
    {
        FixedBackground = -2000, Object = -1000, Graphic = 0, Tool = 1000, Control = 2000
    };
};

#endif // WBCFFCONSTANTS_H
