#include "splittunnelingappswindowitem.h"

#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

SplitTunnelingAppsWindowItem::SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    desc_ = new PreferenceGroup(this);
    desc_->setDescriptionBorderWidth(2);
    addItem(desc_);

    splitTunnelingAppsGroup_ = new SplitTunnelingAppsGroup(this);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::appsUpdated, this, &SplitTunnelingAppsWindowItem::onAppsUpdated);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::addClicked, this, &SplitTunnelingAppsWindowItem::addButtonClicked);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::escape, this, &SplitTunnelingAppsWindowItem::escape);
    addItem(splitTunnelingAppsGroup_);
    setFocusProxy(splitTunnelingAppsGroup_);

    splitTunnelingAppsGroup_->setApps(preferences->splitTunnelingApps());

    setLoggedIn(false);
}

QString SplitTunnelingAppsWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Apps");
}

QList<types::SplitTunnelingApp> SplitTunnelingAppsWindowItem::getApps()
{
    return splitTunnelingAppsGroup_->apps();
}

void SplitTunnelingAppsWindowItem::setApps(QList<types::SplitTunnelingApp> apps)
{
    splitTunnelingAppsGroup_->setApps(apps);
}

void SplitTunnelingAppsWindowItem::addAppManually(types::SplitTunnelingApp app)
{
    splitTunnelingAppsGroup_->addApp(app);
}

void SplitTunnelingAppsWindowItem::onAppsUpdated(QList<types::SplitTunnelingApp> apps)
{
    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps);
}

void SplitTunnelingAppsWindowItem::setLoggedIn(bool loggedIn)
{
    if (loggedIn)
    {
        desc_->clearError();
        desc_->setDescription(tr("Add the apps you wish to include in or exclude from the VPN tunnel below."));
    }
    else
    {
        desc_->setDescription(tr("Please log in to modify split tunneling rules."), true);
    }

    splitTunnelingAppsGroup_->setLoggedIn(loggedIn);
}

} // namespace PreferencesWindow
