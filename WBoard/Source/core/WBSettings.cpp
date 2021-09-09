#include "WBSettings.h"

#include <QtWidgets>

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBCryptoUtils.h"

#include "WB.h"
#include "WBSetting.h"

#include "tools/WBToolsManager.h"

#include "core/memcheck.h"

QPointer<WBSettings> WBSettings::sSingleton = 0;

int WBSettings::pointerDiameter = 40;
int WBSettings::crossSize = 24;
int WBSettings::defaultCrossSize = 24;
int WBSettings::minCrossSize = 12;
int WBSettings::maxCrossSize = 96; //TODO: user-settable?
int WBSettings::colorPaletteSize = 5;
int WBSettings::objectFrameWidth = 20;
int WBSettings::boardMargin = 10;

QString WBSettings::documentGroupName = QString("Subject");
QString WBSettings::documentName = QString("Lesson");
QString WBSettings::documentSize = QString("Size");
QString WBSettings::documentIdentifer = QString("ID");
QString WBSettings::documentVersion = QString("Version");
QString WBSettings::documentUpdatedAt = QString("UpdatedAt");
QString WBSettings::documentPageCount = QString("PageCount");
QString WBSettings::documentDate = QString("date");

QString WBSettings::trashedDocumentGroupNamePrefix = QString("_Trash:");

QString WBSettings::uniboardDocumentNamespaceUri = "http://uniboard.mnemis.com/document";
QString WBSettings::uniboardApplicationNamespaceUri = "http://uniboard.mnemis.com/application";

QString WBSettings::undoCommandTransactionName = "UndoTransaction";

const int WBSettings::sDefaultFontPixelSize = 36;
const char *WBSettings::sDefaultFontFamily = "Arial";

QString WBSettings::currentFileVersion = "4.8.0";

QBrush WBSettings::eraserBrushDarkBackground = QBrush(QColor(127, 127, 127, 80));
QBrush WBSettings::eraserBrushLightBackground = QBrush(QColor(127, 127, 127, 80));

QPen WBSettings::eraserPenDarkBackground = QPen(QColor(255, 255, 255, 127));
QPen WBSettings::eraserPenLightBackground = QPen(QColor(0, 0, 0, 127));

QColor WBSettings::markerCircleBrushColorDarkBackground = QColor(127, 127, 127, 80);
QColor WBSettings::markerCircleBrushColorLightBackground = QColor(127, 127, 127, 80);

QColor WBSettings::markerCirclePenColorDarkBackground = QColor(255, 255, 255, 127);
QColor WBSettings::markerCirclePenColorLightBackground = QColor(0, 0, 0, 127);

QColor WBSettings::penCircleBrushColorDarkBackground = QColor(127, 127, 127, 80);
QColor WBSettings::penCircleBrushColorLightBackground = QColor(127, 127, 127, 80);

QColor WBSettings::penCirclePenColorDarkBackground = QColor(255, 255, 255, 127);
QColor WBSettings::penCirclePenColorLightBackground = QColor(0, 0, 0, 127);

QColor WBSettings::documentSizeMarkColorDarkBackground = QColor(44, 44, 44, 200);
QColor WBSettings::documentSizeMarkColorLightBackground = QColor(241, 241, 241);

QColor WBSettings::paletteColor = QColor(127, 127, 127, 65);
QColor WBSettings::opaquePaletteColor = QColor(66, 66, 66, 200);

QColor WBSettings::documentViewLightColor = QColor(241, 241, 241);

QPointer<QSettings> WBSettings::sAppSettings = 0;

const int WBSettings::maxThumbnailWidth = 400;
const int WBSettings::defaultThumbnailWidth = 100;
const int WBSettings::defaultSortKind = 0;
const int WBSettings::defaultSortOrder = 0;
const int WBSettings::defaultSplitterLeftSize = 200;
const int WBSettings::defaultSplitterRightSize = 800;

const int WBSettings::defaultLibraryIconSize = 80;

const int WBSettings::defaultGipWidth = 150;
const int WBSettings::defaultSoundWidth = 50;
const int WBSettings::defaultImageWidth = 150;
const int WBSettings::defaultShapeWidth = 50;
const int WBSettings::defaultWidgetIconWidth = 110;
const int WBSettings::defaultVideoWidth = 80;

const int WBSettings::thumbnailSpacing = 20;
const int WBSettings::longClickInterval = 1200;

const qreal WBSettings::minScreenRatio = 1.33; // 800/600 or 1024/768

QStringList WBSettings::bitmapFileExtensions;
QStringList WBSettings::vectoFileExtensions;
QStringList WBSettings::imageFileExtensions;
QStringList WBSettings::widgetFileExtensions;
QStringList WBSettings::interactiveContentFileExtensions;

QColor WBSettings::treeViewBackgroundColor = QColor(209, 215, 226); //in synch with css tree view background

int WBSettings::objectInControlViewMargin = 100;

QString WBSettings::appPingMessage = "__uniboard_ping";

WBSettings* WBSettings::settings()
{
    if (!sSingleton) {
        sSingleton = new WBSettings(qApp);
    }
    return sSingleton;
}

void WBSettings::destroy()
{
    if (sSingleton)
        delete sSingleton;
    sSingleton = NULL;
}


QSettings* WBSettings::getAppSettings()
{
    if (!WBSettings::sAppSettings)
    {
        QString tmpSettings = QDir::tempPath() + "/" + qApp->applicationName() + ".config";
        QString appSettings = WBPlatformUtils::applicationResourcesDirectory() + "/etc/" + qApp->applicationName() + ".config";

        // tmpSettings exists when upgrading Uniboard on Mac (see UBPlatformUtils_mac.mm updater:willInstallUpdate:)
        if (QFile::exists(tmpSettings))
        {
            QFile::rename(tmpSettings, appSettings);
        }

        WBSettings::sAppSettings = new QSettings(appSettings, QSettings::IniFormat, 0);
        WBSettings::sAppSettings->setIniCodec("utf-8");

        qDebug() << "sAppSettings location: " << appSettings;
    }

    return WBSettings::sAppSettings;
}


WBSettings::WBSettings(QObject *parent)
    : QObject(parent)
{
    InitKeyboardPaletteKeyBtnSizes();

    mAppSettings = WBSettings::getAppSettings();

    QString userSettingsFile = WBSettings::userDataDirectory() + "/"+qApp->applicationName()+"User.config";

    mUserSettings = new QSettings(userSettingsFile, QSettings::IniFormat, parent);

    init();
}


WBSettings::~WBSettings()
{
    delete mAppSettings;

    if(supportedKeyboardSizes)
        delete supportedKeyboardSizes;
}

void WBSettings::InitKeyboardPaletteKeyBtnSizes()
{
    supportedKeyboardSizes = new QStringList();
    supportedKeyboardSizes->append("29x29");
    supportedKeyboardSizes->append("41x41");
}

void WBSettings::ValidateKeyboardPaletteKeyBtnSize()
{
    // if boardKeyboardPaletteKeyBtnSize is not initialized, or supportedKeyboardSizes not initialized or empty
    if( !boardKeyboardPaletteKeyBtnSize ||
        !supportedKeyboardSizes ||
        supportedKeyboardSizes->size() == 0 ) return;

    // get original size value
    QString origValue = boardKeyboardPaletteKeyBtnSize->get().toString();

    // parse available size values, for make sure original value is valid
    for(int i = 0; i < supportedKeyboardSizes->size(); i++)
    {
        int compareCode = QString::compare(origValue, supportedKeyboardSizes->at(i));
        if(compareCode == 0) return;
    }

    // if original value is invalid, than set it value to first value from avaliable list
    boardKeyboardPaletteKeyBtnSize->set(supportedKeyboardSizes->at(0));
}

void WBSettings::init()
{
    productWebUrl =  new WBSetting(this, "App", "ProductWebAddress", "http://www.baidu.com");

    softwareHomeUrl = productWebUrl->get().toString();

    documentSizes.insert(DocumentSizeRatio::Ratio4_3, QSize(1280, 960)); // 1.33
    documentSizes.insert(DocumentSizeRatio::Ratio16_9, QSize((960 / 9 * 16), 960)); // 1.77

    appToolBarPositionedAtTop = new WBSetting(this, "App", "ToolBarPositionedAtTop", true);
    appToolBarDisplayText = new WBSetting(this, "App", "ToolBarDisplayText", true);
    appEnableAutomaticSoftwareUpdates = new WBSetting(this, "App", "EnableAutomaticSoftwareUpdates", false);
    appSoftwareUpdateURL = new WBSetting(this, "App", "SoftwareUpdateURL", "http://www.baidu.com");
    appHideCheckForSoftwareUpdate = new WBSetting(this, "App", "HideCheckForSoftwareUpdate", false);
    appHideSwapDisplayScreens = new WBSetting(this, "App", "HideSwapDisplayScreens", true);
    appToolBarOrientationVertical = new WBSetting(this, "App", "ToolBarOrientationVertical", false);
    appPreferredLanguage = new WBSetting(this,"App","PreferredLanguage", "");

    rightLibPaletteBoardModeWidth = new WBSetting(this, "Board", "RightLibPaletteBoardModeWidth", 270);
    rightLibPaletteBoardModeIsCollapsed = new WBSetting(this,"Board", "RightLibPaletteBoardModeIsCollapsed",true);
    rightLibPaletteDesktopModeWidth = new WBSetting(this, "Board", "RightLibPaletteDesktopModeWidth", 270);
    rightLibPaletteDesktopModeIsCollapsed = new WBSetting(this,"Board", "RightLibPaletteDesktopModeIsCollapsed",true);
    leftLibPaletteBoardModeWidth = new WBSetting(this, "Board", "LeftLibPaletteBoardModeWidth",270);
    leftLibPaletteBoardModeIsCollapsed = new WBSetting(this,"Board","LeftLibPaletteBoardModeIsCollapsed",true);
    leftLibPaletteDesktopModeWidth = new WBSetting(this, "Board", "LeftLibPaletteDesktopModeWidth",270);
    leftLibPaletteDesktopModeIsCollapsed = new WBSetting(this,"Board","LeftLibPaletteDesktopModeIsCollapsed",true);

    appIsInSoftwareUpdateProcess = new WBSetting(this, "App", "IsInSoftwareUpdateProcess", false);
    appLastSessionDocumentUUID = new WBSetting(this, "App", "LastSessionDocumentUUID", "");
    appLastSessionPageIndex = new WBSetting(this, "App", "LastSessionPageIndex", 0);
    appUseMultiscreen = new WBSetting(this, "App", "UseMultiscreenMode", true);

    appStartupHintsEnabled = new WBSetting(this,"App","EnableStartupHints",true);

    appLookForOpenSankoreInstall = new WBSetting(this, "App", "LookForOpenSankoreInstall", true);

    appStartMode = new WBSetting(this, "App", "StartMode", "");

    featureSliderPosition = new WBSetting(this, "Board", "FeatureSliderPosition", 40);

    boardPenFineWidth = new WBSetting(this, "Board", "PenFineWidth", 1.5);
    boardPenMediumWidth = new WBSetting(this, "Board", "PenMediumWidth", 3.0);
    boardPenStrongWidth = new WBSetting(this, "Board", "PenStrongWidth", 8.0);

    boardMarkerFineWidth = new WBSetting(this, "Board", "MarkerFineWidth", 12.0);
    boardMarkerMediumWidth = new WBSetting(this, "Board", "MarkerMediumWidth", 24.0);
    boardMarkerStrongWidth = new WBSetting(this, "Board", "MarkerStrongWidth", 48.0);

    boardPenPressureSensitive = new WBSetting(this, "Board", "PenPressureSensitive", true);
    boardMarkerPressureSensitive = new WBSetting(this, "Board", "MarkerPressureSensitive", false);

    boardUseHighResTabletEvent = new WBSetting(this, "Board", "UseHighResTabletEvent", true);

    boardInterpolatePenStrokes = new WBSetting(this, "Board", "InterpolatePenStrokes", true);
    boardSimplifyPenStrokes = new WBSetting(this, "Board", "SimplifyPenStrokes", true);
    boardSimplifyPenStrokesThresholdAngle = new WBSetting(this, "Board", "SimplifyPenStrokesThresholdAngle", 2);
    boardSimplifyPenStrokesThresholdWidthDifference = new WBSetting(this, "Board", "SimplifyPenStrokesThresholdWidthDifference", 2.0);

    boardInterpolateMarkerStrokes = new WBSetting(this, "Board", "InterpolateMarkerStrokes", true);
    boardSimplifyMarkerStrokes = new WBSetting(this, "Board", "SimplifyMarkerStrokes", true);

    boardKeyboardPaletteKeyBtnSize = new WBSetting(this, "Board", "KeyboardPaletteKeyBtnSize", "16x16");
    ValidateKeyboardPaletteKeyBtnSize();

    pageSize = new WBSetting(this, "Board", "DefaultPageSize", documentSizes.value(DocumentSizeRatio::Ratio4_3));

    boardCrossColorDarkBackground = new WBSetting(this, "Board", "CrossColorDarkBackground", "#C8C0C0C0");
    boardCrossColorLightBackground = new WBSetting(this, "Board", "CrossColorLightBackground", "#A5E1FF");

    QStringList gridLightBackgroundColors;
    gridLightBackgroundColors << "#000000" << "#FF0000" << "#004080" << "#008000" << "#FFDD00" << "#C87400" << "#800040" << "#008080" << "#A5E1FF";
    boardGridLightBackgroundColors = new WBColorListSetting(this, "Board", "GridLightBackgroundColors", gridLightBackgroundColors, -1.0);

    QStringList gridDarkBackgroundColors;
    gridDarkBackgroundColors << "#FFFFFF" << "#FF3400" << "#66C0FF" << "#81FF5C" << "#FFFF00" << "#B68360" << "#FF497E" << "#8D69FF" << "#C8C0C0C0";
    boardGridDarkBackgroundColors = new WBColorListSetting(this, "Board", "GridDarkBackgroundColors", gridDarkBackgroundColors, -1.0);

    QStringList penLightBackgroundColors;
    penLightBackgroundColors << "#000000" << "#FF0000" <<"#004080" << "#008000" << "#FFDD00" << "#C87400" << "#800040" << "#008080"  << "#5F2D0A" << "#FFFFFF";
    boardPenLightBackgroundColors = new WBColorListSetting(this, "Board", "PenLightBackgroundColors", penLightBackgroundColors, 1.0);

    QStringList penDarkBackgroundColors;
    penDarkBackgroundColors << "#FFFFFF" << "#FF3400" <<"#66C0FF" << "#81FF5C" << "#FFFF00" << "#B68360" << "#FF497E" << "#8D69FF" << "#000000";
    boardPenDarkBackgroundColors = new WBColorListSetting(this, "Board", "PenDarkBackgroundColors", penDarkBackgroundColors, 1.0);

    boardMarkerAlpha = new WBSetting(this, "Board", "MarkerAlpha", 0.5);

    QStringList markerLightBackgroundColors;
    markerLightBackgroundColors << "#E3FF00" << "#FF0000" <<"#004080" << "#008000" << "#C87400" << "#800040" << "#008080"  << "#000000";
    boardMarkerLightBackgroundColors = new WBColorListSetting(this, "Board", "MarkerLightBackgroundColors", markerLightBackgroundColors, boardMarkerAlpha->get().toDouble());

    QStringList markerDarkBackgroundColors;
    markerDarkBackgroundColors << "#FFFF00" << "#FF4400" <<"#66C0FF" << "#81FF5C" << "#B68360" << "#FF497E" << "#8D69FF" << "#FFFFFF";
    boardMarkerDarkBackgroundColors = new WBColorListSetting(this, "Board", "MarkerDarkBackgroundColors", markerDarkBackgroundColors, boardMarkerAlpha->get().toDouble());

    QStringList penLightBackgroundSelectedColors;
    QStringList penDarkBackgroundSelectedColors;

    QStringList markerLightBackgroundSelectedColors;
    QStringList markerDarkBackgroundSelectedColors;

    for (int i = 0; i < colorPaletteSize; i++)
    {
        penLightBackgroundSelectedColors << penLightBackgroundColors[i];
        penDarkBackgroundSelectedColors << penDarkBackgroundColors[i];
        markerLightBackgroundSelectedColors << markerLightBackgroundColors[i];
        markerDarkBackgroundSelectedColors << markerDarkBackgroundColors[i];
    }

    boardPenLightBackgroundSelectedColors = new WBColorListSetting(this, "Board", "PenLightBackgroundSelectedColors", penLightBackgroundSelectedColors);
    boardPenDarkBackgroundSelectedColors = new WBColorListSetting(this, "Board", "PenDarkBackgroundSelectedColors", penDarkBackgroundSelectedColors);

    boardMarkerLightBackgroundSelectedColors = new WBColorListSetting(this, "Board", "MarkerLightBackgroundSelectedColors", markerLightBackgroundSelectedColors, boardMarkerAlpha->get().toDouble());
    boardMarkerDarkBackgroundSelectedColors = new WBColorListSetting(this, "Board", "MarkerDarkBackgroundSelectedColors", markerDarkBackgroundSelectedColors, boardMarkerAlpha->get().toDouble());

    showEraserPreviewCircle = new WBSetting(this, "Board", "ShowEraserPreviewCircle", true);
    showMarkerPreviewCircle = new WBSetting(this, "Board", "ShowMarkerPreviewCircle", true);
    showPenPreviewCircle = new WBSetting(this, "Board", "ShowPenPreviewCircle", true);
    penPreviewFromSize = new WBSetting(this, "Board", "PenPreviewFromSize", 5);

    webUseExternalBrowser = new WBSetting(this, "Web", "UseExternalBrowser", false);

    bool defaultShowPageImmediatelyOnMirroredScreen = true;

#if defined(Q_OS_LINUX)
    // screen duplication is very slow on X11
    defaultShowPageImmediatelyOnMirroredScreen = false;
#endif

    webShowPageImmediatelyOnMirroredScreen = new WBSetting(this, "Web", "ShowPageImediatelyOnMirroredScreen", defaultShowPageImmediatelyOnMirroredScreen);

    webHomePage = new WBSetting(this, "Web", "Homepage", softwareHomeUrl);
    webBookmarksPage = new WBSetting(this, "Web", "BookmarksPage", "http://www.baidu.com");
    webAddBookmarkUrl = new WBSetting(this, "Web", "AddBookmarkURL", "http://www.baidu.com");
    webShowAddBookmarkButton = new WBSetting(this, "Web", "ShowAddBookmarkButton", false);

    pageCacheSize = new WBSetting(this, "App", "PageCacheSize", 20);

    bitmapFileExtensions << "jpg" << "jpeg" <<  "png" <<  "tiff" << "tif" << "bmp" << "gif";
    vectoFileExtensions << "svg" <<  "svgz";
    imageFileExtensions << bitmapFileExtensions << vectoFileExtensions;

    widgetFileExtensions << "wdgt" << "wgt" << "pwgt";
    interactiveContentFileExtensions << widgetFileExtensions << "swf";

    boardZoomFactor = new WBSetting(this, "Board", "ZoomFactor", QVariant(1.41));

    if (boardZoomFactor->get().toDouble() <= 1.)
        boardZoomFactor->set(1.41);

    int defaultRefreshRateInFramePerSecond = 8;

#if defined(Q_OS_LINUX)
    // screen duplication is very slow on X11
    defaultRefreshRateInFramePerSecond = 2;
#endif

    mirroringRefreshRateInFps = new WBSetting(this, "Mirroring", "RefreshRateInFramePerSecond", QVariant(defaultRefreshRateInFramePerSecond));

    lastImportFilePath = new WBSetting(this, "Import", "LastImportFilePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastImportFolderPath = new WBSetting(this, "Import", "LastImportFolderPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastExportFilePath = new WBSetting(this, "Export", "LastExportFilePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastExportDirPath = new WBSetting(this, "Export", "LastExportDirPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastImportToLibraryPath = new WBSetting(this, "Library", "LastImportToLibraryPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));

    lastPicturePath = new WBSetting(this, "Library", "LastPicturePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)));
    lastWidgetPath = new WBSetting(this, "Library", "LastWidgetPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastVideoPath = new WBSetting(this, "Library", "LastVideoPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)));

    appOnlineUserName = new WBSetting(this, "App", "OnlineUserName", "");

    boardShowToolsPalette = new WBSetting(this, "Board", "ShowToolsPalette", "false");
    magnifierDrawingMode = new WBSetting(this, "Board", "MagnifierDrawingMode", "0");
    autoSaveInterval = new WBSetting(this, "Board", "AutoSaveIntervalInMinutes", "3");

    svgViewBoxMargin = new WBSetting(this, "SVG", "ViewBoxMargin", "50");

    pdfMargin = new WBSetting(this, "PDF", "Margin", "20");
    pdfPageFormat = new WBSetting(this, "PDF", "PageFormat", "A4");
    pdfResolution = new WBSetting(this, "PDF", "Resolution", "300");

    podcastFramesPerSecond = new WBSetting(this, "Podcast", "FramesPerSecond", 10);
    podcastVideoSize = new WBSetting(this, "Podcast", "VideoSize", "Medium");
    podcastAudioRecordingDevice = new WBSetting(this, "Podcast", "AudioRecordingDevice", "Default");

    podcastWindowsMediaBitsPerSecond = new WBSetting(this, "Podcast", "WindowsMediaBitsPerSecond", 1700000);
    podcastQuickTimeQuality = new WBSetting(this, "Podcast", "QuickTimeQuality", "High");

    communityUser = new WBSetting(this, "Community", "Username", "");
    communityPsw = new WBSetting(this, "Community", "Password", "");
    communityCredentialsPersistence = new WBSetting(this,"Community", "CredentialsPersistence",false);

    QStringList uris = WBToolsManager::manager()->allToolIDs();

    favoritesNativeToolUris = new WBSetting(this, "App", "FavoriteToolURIs", uris);

    // removed in version 4.4.b.2
    mUserSettings->remove("Podcast/RecordMicrophone");

    documentThumbnailWidth      = new WBSetting(this, "Document", "ThumbnailWidth", WBSettings::defaultThumbnailWidth);
    documentSortKind            = new WBSetting(this, "Document", "SortKind", WBSettings::defaultSortKind);
    documentSortOrder           = new WBSetting(this, "Document", "SortOrder", WBSettings::defaultSortOrder);
    documentSplitterLeftSize    = new WBSetting(this, "Document", "SplitterLeftSize", WBSettings::defaultSplitterLeftSize);
    documentSplitterRightSize   = new WBSetting(this, "Document", "SplitterRightSize", WBSettings::defaultSplitterRightSize);

    libraryShowDetailsForLocalItems = new WBSetting(this, "Library", "ShowDetailsForLocalItems", false);

    imageThumbnailWidth = new WBSetting(this, "Library", "ImageThumbnailWidth", WBSettings::defaultImageWidth);
    videoThumbnailWidth = new WBSetting(this, "Library", "VideoThumbnailWidth", WBSettings::defaultVideoWidth);
    shapeThumbnailWidth = new WBSetting(this, "Library", "ShapeThumbnailWidth", WBSettings::defaultShapeWidth);
    gipThumbnailWidth = new WBSetting(this, "Library", "ImageThumbnailWidth", WBSettings::defaultGipWidth);
    soundThumbnailWidth = new WBSetting(this, "Library", "SoundThumbnailWidth", WBSettings::defaultSoundWidth);;

    KeyboardLocale = new WBSetting(this, "Board", "StartupKeyboardLocale", 0);
    swapControlAndDisplayScreens = new WBSetting(this, "App", "SwapControlAndDisplayScreens", false);

    angleTolerance = new WBSetting(this, "App", "AngleTolerance", 4);
    historyLimit = new WBSetting(this, "Web", "HistoryLimit", 15);

    libIconSize = new WBSetting(this, "Library", "LibIconSize", defaultLibraryIconSize);

    useSystemOnScreenKeyboard = new WBSetting(this, "App", "UseSystemOnScreenKeyboard", true);

    showDateColumnOnAlphabeticalSort = new WBSetting(this, "Document", "ShowDateColumnOnAlphabeticalSort", false);
    emptyTrashForOlderDocuments = new WBSetting(this, "Document", "emptyTrashForOlderDocuments", false);
    emptyTrashDaysValue = new WBSetting(this, "Document", "emptyTrashDaysValue", 30);

    cleanNonPersistentSettings();
    checkNewSettings();
}


/**
 * @brief Returns the value for the *key* setting, or *defaultValue* if the key doesn't exist
 *
 * The value is also added to the local settings queue, to prevent future disk I/O when accessing
 * that same setting.
 *
 * If the value doesn't exist in the application settings (i.e it was not present in the config file),
 * it is also added there.
 */
QVariant WBSettings::value ( const QString & key, const QVariant & defaultValue)
{
    // Check first the settings queue, then the app settings, then the user settings.
    // If the key exists in neither of these, then defaultValue is returned.

    if (mSettingsQueue.contains(key))
        return mSettingsQueue.value(key);

    // If the setting doesn't exist in the App settings, add it there
    if (!sAppSettings->contains(key) && !(defaultValue == QVariant()))
        sAppSettings->setValue(key, defaultValue);

    // Get the value from the user settings (or if it doesn't exist there, from the app settings)
    QVariant val = mUserSettings->value(key, sAppSettings->value(key, defaultValue));

    // Add the value to the settings queue for faster access next time it is requested
    mSettingsQueue[key] = val;

    return val;
}


void WBSettings::setValue (const QString & key, const QVariant & value)
{
    // Save the setting to the queue only; a call to save() is necessary to persist the settings
    mSettingsQueue[key] = value;
}

/**
 * @brief Save all the queued settings to disk
 */
void WBSettings::save()
{
    QHash<QString, QVariant>::const_iterator it = mSettingsQueue.constBegin();

    while (it != mSettingsQueue.constEnd()) {
        /*
         * We save the setting to the user settings if
         * a) it is different from the (non-null) value stored in the user settings, or
         * b) it doesn't currently exist in the user settings AND has changed from the app settings
        */
        if (mUserSettings->contains(it.key())
                && it.value() != mUserSettings->value(it.key()))
        {
            mUserSettings->setValue(it.key(), it.value());
        }

        else if (!mUserSettings->contains(it.key())
                 && it.value() != mAppSettings->value(it.key()))
        {
            mUserSettings->setValue(it.key(), it.value());
        }

        ++it;
    }

    // Force save to file
    mUserSettings->sync();

    qDebug() << "User settings saved";
}

int WBSettings::penWidthIndex()
{
    return value("Board/PenLineWidthIndex", 0).toInt();
}


void WBSettings::setPenWidthIndex(int index)
{
    if (index != penWidthIndex())
    {
        setValue("Board/PenLineWidthIndex", index);
    }
}


qreal WBSettings::currentPenWidth()
{
    qreal width = 0;

    switch (penWidthIndex())
    {
        case WBWidth::Fine:
            width = boardPenFineWidth->get().toDouble();
            break;
        case WBWidth::Medium:
            width = boardPenMediumWidth->get().toDouble();
            break;
        case WBWidth::Strong:
            width = boardPenStrongWidth->get().toDouble();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = boardPenFineWidth->get().toDouble();
            break;
    }

    return width;
}


int WBSettings::penColorIndex()
{
    return value("Board/PenColorIndex", 0).toInt();
}


void WBSettings::setPenColorIndex(int index)
{
    if (index != penColorIndex())
    {
        setValue("Board/PenColorIndex", index);
    }
}


QColor WBSettings::currentPenColor()
{
    return penColor(isDarkBackground());
}


QColor WBSettings::penColor(bool onDarkBackground)
{
    QList<QColor> colors = penColors(onDarkBackground);
    return colors.at(penColorIndex());
}


QList<QColor> WBSettings::penColors(bool onDarkBackground)
{
    if (onDarkBackground)
    {
        return boardPenDarkBackgroundSelectedColors->colors();
    }
    else
    {
        return boardPenLightBackgroundSelectedColors->colors();
    }
}


int WBSettings::markerWidthIndex()
{
    return value("Board/MarkerLineWidthIndex", 0).toInt();
}


void WBSettings::setMarkerWidthIndex(int index)
{
    if (index != markerWidthIndex())
    {
        setValue("Board/MarkerLineWidthIndex", index);
    }
}


qreal WBSettings::currentMarkerWidth()
{
    qreal width = 0;

    switch (markerWidthIndex())
    {
        case WBWidth::Fine:
            width = boardMarkerFineWidth->get().toDouble();
            break;
        case WBWidth::Medium:
            width = boardMarkerMediumWidth->get().toDouble();
            break;
        case WBWidth::Strong:
            width = boardMarkerStrongWidth->get().toDouble();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = boardMarkerFineWidth->get().toDouble();
            break;
    }

    return width;
}


int WBSettings::markerColorIndex()
{
    return value("Board/MarkerColorIndex", 0).toInt();
}


void WBSettings::setMarkerColorIndex(int index)
{
    if (index != markerColorIndex())
    {
        setValue("Board/MarkerColorIndex", index);
    }
}


QColor WBSettings::currentMarkerColor()
{
    return markerColor(isDarkBackground());
}


QColor WBSettings::markerColor(bool onDarkBackground)
{
    QList<QColor> colors = markerColors(onDarkBackground);
    return colors.at(markerColorIndex());
}


QList<QColor> WBSettings::markerColors(bool onDarkBackground)
{
    if (onDarkBackground)
    {
        return boardMarkerDarkBackgroundSelectedColors->colors();
    }
    else
    {
        return boardMarkerLightBackgroundSelectedColors->colors();
    }
}

//----------------------------------------//
// eraser

int WBSettings::eraserWidthIndex()
{
    return value("Board/EraserCircleWidthIndex", 1).toInt();
}

void WBSettings::setEraserWidthIndex(int index)
{
    setValue("Board/EraserCircleWidthIndex", index);
}

qreal WBSettings::eraserFineWidth()
{
    return value("Board/EraserFineWidth", 16).toDouble();
}

void WBSettings::setEraserFineWidth(qreal width)
{
    setValue("Board/EraserFineWidth", width);
}

qreal WBSettings::eraserMediumWidth()
{
    return value("Board/EraserMediumWidth", 64).toDouble();
}

void WBSettings::setEraserMediumWidth(qreal width)
{
    setValue("Board/EraserMediumWidth", width);
}

qreal WBSettings::eraserStrongWidth()
{
    return value("Board/EraserStrongWidth", 128).toDouble();
}

void WBSettings::setEraserStrongWidth(qreal width)
{
    setValue("Board/EraserStrongWidth", width);
}

qreal WBSettings::currentEraserWidth()
{
    qreal width = 0;

    switch (eraserWidthIndex())
    {
        case WBWidth::Fine:
            width = eraserFineWidth();
            break;
        case WBWidth::Medium:
            width = eraserMediumWidth();
            break;
        case WBWidth::Strong:
            width = eraserStrongWidth();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = eraserFineWidth();
            break;
    }

    return width;
}

bool WBSettings::isDarkBackground()
{
    return value("Board/DarkBackground", 0).toBool();
}


WBPageBackground WBSettings::pageBackground()
{
    QString val = value("Board/PageBackground", 0).toString();

    if (val == "crossed")
        return WBPageBackground::crossed;
    else if (val == "ruled")
        return WBPageBackground::ruled;
    else
        return WBPageBackground::plain;
}


void WBSettings::setDarkBackground(bool isDarkBackground)
{
    setValue("Board/DarkBackground", isDarkBackground);
    emit colorContextChanged();
}


void WBSettings::setPageBackground(WBPageBackground background)
{
    QString val;

    if (background == WBPageBackground::crossed)
        val = "crossed";
    else if (background == WBPageBackground::ruled)
        val = "ruled";
    else
        val = "plain";

    setValue("Board/PageBackground", val);
}


void WBSettings::setPenPressureSensitive(bool sensitive)
{
    boardPenPressureSensitive->set(sensitive);
}

void WBSettings::setPenPreviewCircle(bool circle)
{
    showPenPreviewCircle->set(circle);
}

void WBSettings::setPenPreviewFromSize(int size)
{
    penPreviewFromSize->set(size);
}

void WBSettings::setMarkerPressureSensitive(bool sensitive)
{
    boardMarkerPressureSensitive->set(sensitive);
}


bool WBSettings::isStylusPaletteVisible()
{
    return value("Board/StylusPaletteIsVisible", true).toBool();
}


void WBSettings::setStylusPaletteVisible(bool visible)
{
    setValue("Board/StylusPaletteIsVisible", visible);
}


QString WBSettings::fontFamily()
{
    return value("Board/FontFamily", sDefaultFontFamily).toString();
}


void WBSettings::setFontFamily(const QString &family)
{
    setValue("Board/FontFamily", family);
}


int WBSettings::fontPixelSize()
{
    return value("Board/FontPixelSize", sDefaultFontPixelSize).toInt();
}


void WBSettings::setFontPixelSize(int pixelSize)
{
    setValue("Board/FontPixelSize", pixelSize);
}

int WBSettings::fontPointSize()
{
    return value("Board/FontPointSize", 12).toInt();
}

void WBSettings::setFontPointSize(int pointSize)
{
    setValue("Board/FontPointSize", pointSize);
}

bool WBSettings::isBoldFont()
{
    return value("Board/FontIsBold", false).toBool();
}


void WBSettings::setBoldFont(bool bold)
{
    setValue("Board/FontIsBold", bold);
}


bool WBSettings::isItalicFont()
{
    return value("Board/FontIsItalic", false).toBool();
}


void WBSettings::setItalicFont(bool italic)
{
    setValue("Board/FontIsItalic", italic);
}


QString WBSettings::userDataDirectory()
{
    static QString dataDirPath = "";
    if(dataDirPath.isEmpty()){
        if (getAppSettings() && getAppSettings()->contains("App/DataDirectory")) {
            qDebug() << "getAppSettings()->contains(App/DataDirectory):" << getAppSettings()->contains("App/DataDirectory");
            dataDirPath = getAppSettings()->value("App/DataDirectory").toString();
            dataDirPath = replaceWildcard(dataDirPath);

            if(checkDirectory(dataDirPath))
                return dataDirPath;
            else
                qCritical() << "Impossible to create datadirpath " << dataDirPath;

        }
        dataDirPath = WBFileSystemUtils::normalizeFilePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if (qApp->organizationName().size() > 0)
            dataDirPath.replace(qApp->organizationName() + "/", "");
    }
    return dataDirPath;
}


QString WBSettings::userImageDirectory()
{
    static QString imageDirectory = "";
    if(imageDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserImageDirectory")) {
            imageDirectory = getAppSettings()->value("App/UserImageDirectory").toString();

            imageDirectory = replaceWildcard(imageDirectory);
            if(checkDirectory(imageDirectory))
                return imageDirectory;
            else
                qCritical() << "failed to create image directory " << imageDirectory;
        }

        imageDirectory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/" + qApp->applicationName();
        checkDirectory(imageDirectory);
    }
    return imageDirectory;
}


QString WBSettings::userVideoDirectory()
{
    static QString videoDirectory = "";
    if(videoDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserVideoDirectory")) {
            videoDirectory = getAppSettings()->value("App/UserVideoDirectory").toString();
            videoDirectory = replaceWildcard(videoDirectory);
            if(checkDirectory(videoDirectory))
                return videoDirectory;
            else
                qCritical() << "failed to create video directory " << videoDirectory;
        }


        videoDirectory = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

        if(videoDirectory.isEmpty())
            videoDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + tr("My Movies");
        else
            videoDirectory = videoDirectory + "/" + qApp->applicationName();

        checkDirectory(videoDirectory);
    }
    return videoDirectory;
}

QString WBSettings::userAudioDirectory()
{
    static QString audioDirectory = "";
    if(audioDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserAudioDirectory")) {
            audioDirectory = getAppSettings()->value("App/UserAudioDirectory").toString();

            audioDirectory = replaceWildcard(audioDirectory);
            if(checkDirectory(audioDirectory))
                return audioDirectory;
            else
                qCritical() << "failed to create image directory " << audioDirectory;
        }

        audioDirectory = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + qApp->applicationName();
        checkDirectory(audioDirectory);
    }
    return audioDirectory;
}


QString WBSettings::userPodcastRecordingDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("Podcast/RecordingDirectory"))
        {
            dirPath = getAppSettings()->value("Podcast/RecordingDirectory").toString();
            dirPath = replaceWildcard(dirPath);
            if(checkDirectory(dirPath))
                return dirPath;
            else
                qCritical() << "failed to create dir " << dirPath;

        }
        dirPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        checkDirectory(dirPath);
    }
    return dirPath;
}

QString WBSettings::userDocumentDirectory()
{
    static QString documentDirectory = "";
    if(documentDirectory.isEmpty()){
        documentDirectory = userDataDirectory() + "/document";
        checkDirectory(documentDirectory);
    }
    qDebug() << "userDocumentDirectory()" << documentDirectory;
    return documentDirectory;
}

QString WBSettings::userFavoriteListFilePath()
{
    static QString filePath = "";
    if(filePath.isEmpty()){
        QString dirPath = userDataDirectory() + "/libraryPalette";
        checkDirectory(dirPath);
        filePath = dirPath + "/favorite.dat";
    }
    return filePath;
}

QString WBSettings::userTrashDirPath()
{
    static QString trashPath = "";
    if(trashPath.isEmpty()){
        trashPath = userDataDirectory() + "/libraryPalette/trash";
        checkDirectory(trashPath);
    }
    return trashPath;
}


QString WBSettings::userGipLibraryDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        dirPath = userDataDirectory() + "/library/gips";
        checkDirectory(dirPath);
    }
    return dirPath;
}


QString WBSettings::applicationShapeLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/shape");

    QString configPath = value("Library/ShapeDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::applicationCustomizationDirectory()
{
    QString defaultRelativePath = QString("/customizations");
    return WBPlatformUtils::applicationResourcesDirectory() + defaultRelativePath;

}

QString WBSettings::applicationCustomFontDirectory()
{
    QString defaultFontDirectory = "/fonts";
    return applicationCustomizationDirectory() + defaultFontDirectory;
}

QString WBSettings::userSearchDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        dirPath = WBPlatformUtils::applicationResourcesDirectory() + "/library/search";
        checkDirectory(dirPath);
    }
    return dirPath;
}

QString WBSettings::applicationImageLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/pictures");

    QString configPath = value("Library/ImageDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::userAnimationDirectory()
{
    static QString animationDirectory = "";
    if(animationDirectory.isEmpty()){
        animationDirectory = userDataDirectory() + "/animationUserDirectory";
        checkDirectory(animationDirectory);
    }
    return animationDirectory;
}

QString WBSettings::userInteractiveDirectory()
{
    static QString interactiveDirectory = "";
    if(interactiveDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserInteractiveContentDirectory")) {
            interactiveDirectory = getAppSettings()->value("App/UserInteractiveContentDirectory").toString();
            interactiveDirectory = replaceWildcard(interactiveDirectory);
            if(checkDirectory(interactiveDirectory))
                return interactiveDirectory;
            else
                qCritical() << "failed to create directory " << interactiveDirectory;
        }
        interactiveDirectory = userDataDirectory() + "/interactive content";
        checkDirectory(interactiveDirectory);
    }
    return interactiveDirectory;
}


QString WBSettings::applicationInteractivesDirectory()
{
    QString defaultRelativePath = QString("./library/interactivities");

    QString configPath = value("Library/InteractivitiesDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::applicationApplicationsLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/applications");

    QString configPath = value("Library/ApplicationsDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}


QString WBSettings::applicationAudiosLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/audios");

    QString configPath = value("Library/AudiosDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::applicationVideosLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/videos");

    QString configPath = value("Library/VideosDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::applicationAnimationsLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/animations");

    QString configPath = value("Library/AnimationsDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString WBSettings::applicationStartupHintsDirectory()
{
    QString defaultRelativePath = QString("./startupHints");

    QString configPath = value("StartupHintsDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return WBPlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else
        return configPath;
}

QString WBSettings::userInteractiveFavoritesDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserInteractiveFavoritesDirectory")) {
            dirPath = getAppSettings()->value("App/UserInteractiveFavoritesDirectory").toString();
            dirPath = replaceWildcard(dirPath);
            if(checkDirectory(dirPath))
                return dirPath;
            else
                qCritical() << "failed to create directory " << dirPath;
        }

        dirPath = userDataDirectory() + "/interactive favorites";
        checkDirectory(dirPath);
    }
    return dirPath;
}


QNetworkProxy* WBSettings::httpProxy()
{
    QNetworkProxy* proxy = 0;

    if (mAppSettings->value("Proxy/Enabled", false).toBool()) {

        proxy = new QNetworkProxy();

        if (mAppSettings->value("Proxy/Type", "HTTP").toString() == "Socks5")
            proxy->setType(QNetworkProxy::Socks5Proxy);
        else
            proxy->setType(QNetworkProxy::HttpProxy);

        proxy->setHostName(mAppSettings->value("Proxy/HostName").toString());
        proxy->setPort(mAppSettings->value("Proxy/Port", 1080).toInt());
        proxy->setUser(mAppSettings->value("Proxy/UserName").toString());
        proxy->setPassword(mAppSettings->value("Proxy/Password").toString());
    }

    return proxy;
}


void WBSettings::setPassword(const QString& id, const QString& password)
{
    QString encrypted = WBCryptoUtils::instance()->symetricEncrypt(password);

    mUserSettings->setValue(QString("Vault/") + id, encrypted);
}


void WBSettings::removePassword(const QString& id)
{
    mUserSettings->remove(QString("Vault/") + id);
}


QString WBSettings::password(const QString& id)
{
    QString encrypted = mUserSettings->value(QString("Vault/") + id).toString();

    QString result = "";

    if (encrypted.length() > 0)
        result =  WBCryptoUtils::instance()->symetricDecrypt(encrypted);

    return result;
}


QString WBSettings::proxyUsername()
{
    QString idUsername = "http.proxy.user";
    return password(idUsername);
}


void WBSettings::setProxyUsername(const QString& username)
{
    QString idUsername = "http.proxy.user";

    if (username.length() > 0)
        setPassword(idUsername, username);
    else
        removePassword(idUsername);
}


QString WBSettings::proxyPassword()
{
    QString idPassword = "http.proxy.pass";
    return password(idPassword);
}


void WBSettings::setProxyPassword(const QString& password)
{
    QString idPassword = "http.proxy.pass";

    if (password.length() > 0)
       setPassword(idPassword, password);
    else
        removePassword(idPassword);

}

QString WBSettings::communityUsername()
{
    return communityUser->get().toString();
}

void WBSettings::setCommunityUsername(const QString &username)
{
    communityUser->set(QVariant(username));
}

QString WBSettings::communityPassword()
{
    return communityPsw->get().toString();
}

void WBSettings::setCommunityPassword(const QString &password)
{
    communityPsw->set(QVariant(password));
}

void WBSettings::setCommunityPersistence(const bool persistence)
{
    communityCredentialsPersistence->set(QVariant(persistence));
}

int WBSettings::libraryIconSize(){
    return libIconSize->get().toInt();
}

void WBSettings::setLibraryIconsize(const int& size){
    libIconSize->set(QVariant(size));
}

bool WBSettings::checkDirectory(QString& dirPath)
{
    bool result = true;
    QDir dir(dirPath);
    if(!dir.exists())
        result = dir.mkpath(dirPath);
    return result;
}

QString WBSettings::replaceWildcard(QString& path)
{
    QString result(path);

    if (result.startsWith("{Documents}")) {
        result = result.replace("{Documents}", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    else if(result.startsWith("{Home}")) {
        result = result.replace("{Home}", QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    }
    else if(result.startsWith("{Desktop}")) {
        result = result.replace("{Desktop}", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    }

    if(result.contains("{UserLoginName}") && WBPlatformUtils::osUserLoginName().length() > 0) {
        result = result.replace("{UserLoginName}", WBPlatformUtils::osUserLoginName());
    }

    return result;
}

void WBSettings::closing()
{
    save();
    cleanNonPersistentSettings();
}

void WBSettings::cleanNonPersistentSettings()
{
    if(!communityCredentialsPersistence->get().toBool()){
        communityPsw->set(QVariant(""));
        communityUser->set(QVariant(""));
    }

}

/**
 * @brief Permanently remove a setting, from local memory and config files
 * @param setting The setting to remove
 */
void WBSettings::removeSetting(const QString &setting)
{
    if (sAppSettings->contains(setting))
        sAppSettings->remove(setting);

    if (mUserSettings->contains(setting))
        mUserSettings->remove(setting);

    if (mSettingsQueue.contains(setting))
        mSettingsQueue.remove(setting);
}

void WBSettings::checkNewSettings()
{
    /*
     * Some settings were modified in new versions and WBoard can crash
     * if an old settings file is used. This function checks these settings and
     * if necessary, resets them to the values initialized in WBSettings::init().
     *
     * Thus this method can be removed when it is no longer deemed useful.
     */

    // OB 1.3 introduced an extra pen color;  for simplicity, if the old settings
    // are still present (i.e only 4 selected pen colors), we just reset all color settings.
    // Having too few colors actually causes WBoard to crash, hence these measures.

    QList<WBColorListSetting*> colorSettings;
    colorSettings << boardPenDarkBackgroundSelectedColors
                  << boardPenLightBackgroundSelectedColors
                  << boardMarkerDarkBackgroundSelectedColors
                  << boardMarkerLightBackgroundSelectedColors;

    foreach (WBColorListSetting* setting, colorSettings) {
        if (setting->colors().size() < 5)
            setting->reset();
    }

    colorSettings.clear();

    // Next we check whether all the new colors were added; i.e if some default
    // colors are not also in the config file, we add them to it.

    // This is not nearly as critical as the above issue, but the users (or admins) seemingly
    // can't be trusted to delete or modify their own config file when upgrading, so they might
    // wonder why they don't have the new colors in their app.

    colorSettings << boardPenDarkBackgroundColors
                  << boardPenLightBackgroundColors
                  << boardMarkerDarkBackgroundColors
                  << boardMarkerLightBackgroundColors;

    foreach (WBColorListSetting* setting, colorSettings) {
        QStringList defaultColors = qvariant_cast<QStringList>(setting->defaultValue());
        QStringList currentColors(qvariant_cast<QStringList>(setting->get())); // copy

        foreach (QString c, defaultColors) {
            if (!currentColors.contains(c))
                currentColors.append(QString(c));
        }

        setting->set(currentColors);
    }


    // A typo was corrected in version 1.3
    removeSetting("Board/useSystemOnScreenKeybard");


    // CrossedBackground changed in 1.4 (no longer a bool but an enum; can be crossed or ruled)
    removeSetting("Board/CrossedBackground");

}
