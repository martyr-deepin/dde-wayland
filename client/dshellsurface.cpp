#include "dshellsurface.h"

#include <qwayland-dde-shell.h>
#include <QWindow>
#include <QGuiApplication>
#include <QDebug>
#include <QWaylandClientExtensionTemplate>
#include <QPlatformSurfaceEvent>
#include <qpa/qplatformnativeinterface.h>

#define SIGNAL_PREFIX "__DWAYLAND_SIGNAL_"

namespace DWaylandClient {

inline struct ::wl_surface *getWlSurface(QWindow *window)
{
    void *surf = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window);
    return static_cast<struct ::wl_surface *>(surf);
}

class DShellSurfacePrivate : public QWaylandClientExtensionTemplate<DShellSurfacePrivate>, public QtWayland::dde_shell_surface
{
public:
    DShellSurfacePrivate(struct ::dde_shell_surface *object)
        : QWaylandClientExtensionTemplate(1)
        , QtWayland::dde_shell_surface(object)
    {

    }

    void dde_shell_surface_geometry(int32_t x, int32_t y, int32_t w, int32_t h) override
    {
        QRect geometry(x, y, w, h);

        if (geometry == this->geometry)
            return;

        this->geometry = geometry;

        Q_Q(DShellSurface);
        Q_EMIT q->geometryChanged(geometry);
    }
    void dde_shell_surface_property(const QString &name, wl_array *value) override
    {
        const QByteArray data(static_cast<char *>(value->data), value->size * 4);
        QDataStream ds(data);
        QVariant vv;
        ds >> vv;

        Q_Q(DShellSurface);

        // 处理信号通知
        if (Q_UNLIKELY(name.startsWith(SIGNAL_PREFIX))) {
            Q_EMIT q->notify(name.mid(strlen(SIGNAL_PREFIX)), vv);
        } else {
            properies[name] = vv;
            Q_EMIT q->propertyChanged(name, vv);
        }
    }

    void set_property(const QString &name, const QVariant &value)
    {
        QByteArray data;
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << value;
        QtWayland::dde_shell_surface::set_property(name, data);
    }

    void requestActivate()
    {
        QtWayland::dde_shell_surface::request_activate();
    }

    DShellSurface *q_ptr;
    QRect geometry;
    QVariantMap properies;

    wl_surface *surface = nullptr;
    QWindow *window = nullptr;

    Q_DECLARE_PUBLIC(DShellSurface)
};

class DShellSurfaceManagerPrivate : public QWaylandClientExtensionTemplate<DShellSurfaceManagerPrivate>, public QtWayland::dde_surface_manager
{
public:
    DShellSurfaceManagerPrivate(DShellSurfaceManager *qq)
        : QWaylandClientExtensionTemplate(1)
        , q_ptr(qq)
    {

    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::PlatformSurface) {
            auto pe = static_cast<QPlatformSurfaceEvent*>(event);

            if (pe->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
                if (QWindow *w = qobject_cast<QWindow*>(watched)) {
                    Q_Q(DShellSurfaceManager);
                    q->registerWindow(w);
                }

                watched->removeEventFilter(this);
            }
        }

        return false;
    }

    DShellSurface *createShellSurface(wl_surface *surface)
    {
        auto dde_surface = get_surface(surface);
        auto s = new DShellSurface(*new DShellSurfacePrivate(dde_surface), this);
        s->d_ptr->surface = surface;
        surfaceMap[surface] = s;
        return s;
    }

    DShellSurfaceManager *q_ptr;
    QHash<wl_surface*, DShellSurface*> surfaceMap;
    Q_DECLARE_PUBLIC(DShellSurfaceManager)
};

DShellSurfaceManager::DShellSurfaceManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new DShellSurfaceManagerPrivate(this))
{
    connect(d_ptr.data(), &DShellSurfaceManagerPrivate::activeChanged, this, &DShellSurfaceManager::activeChanged);
}

DShellSurfaceManager::~DShellSurfaceManager()
{
    Q_D(DShellSurfaceManager);
    d->surfaceMap.clear();
}

bool DShellSurfaceManager::isActive() const
{
    Q_D(const DShellSurfaceManager);
    return d->isActive();
}

DShellSurface *DShellSurfaceManager::ensureShellSurface(wl_surface *surface)
{
    if (!surface)
        return nullptr;

    Q_D(DShellSurfaceManager);

    if (!d->isActive())
        return nullptr;

    if (auto s = d->surfaceMap.value(surface)) {
        return s;
    }

    //如果没有找到surface,应该返回空,由registerWindow创建
    return nullptr;
}

DShellSurface *DShellSurfaceManager::registerWindow(QWindow *window)
{
    Q_D(DShellSurfaceManager);

    if (!d->isActive()) {
        connect(d, &DShellSurfaceManagerPrivate::activeChanged, window, [this, d, window] {
            Q_ASSERT_X(d->isActive(), "DShellSurfaceManager::registerWindow", "The surface manager is not active.");
            registerWindow(window);
        });

        return nullptr;
    }

     // TODO: window可能是野指针, 调用handle会引起崩溃,暂时用if(1)替代
//    if (window->handle()) {
    if (1) {
        auto surface = getWlSurface(window);

        if (!surface)
            return nullptr;

        if (auto s = d->surfaceMap.value(surface)) {
            return s;
        }

        auto s = d->createShellSurface(surface);
        s->d_ptr->window = window;
        // 跟随窗口销毁
        //暂时注释掉
        //connect(window, &QWindow::destroyed, s, &DShellSurface::deleteLater);
        Q_EMIT surfaceCreated(s, window);
    }

    // watch the window surface created
    d->installEventFilter(this);

    return nullptr;
}

DShellSurface::~DShellSurface()
{
    if (auto manager = static_cast<DShellSurfaceManagerPrivate*>(parent())) {
        manager->surfaceMap.remove(manager->surfaceMap.key(this));
    }
}

wl_surface *DShellSurface::surface() const
{
    Q_D(const DShellSurface);
    return d->surface;
}

QWindow *DShellSurface::window() const
{
    Q_D(const DShellSurface);
    return d->window;
}

QVariant DShellSurface::property(const QString &name) const
{
    Q_D(const DShellSurface);

    if (!d->properies.contains(name)) {
        // request get property
        const_cast<DShellSurfacePrivate *>(d)->get_property(name);
        return QVariant();
    }

    return d->properies[name];
}

static bool qvariantComparison(const QVariant &v1, const QVariant &v2)
{
    if (v1.type() != v2.type())
        return false;

    return v1 == v2;
}

void DShellSurface::setProperty(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

    if (qvariantComparison(d->properies.value(name), value)) {
        return;
    }

    d->properies[name] = value;
    d->set_property(name, value);
}

void DShellSurface::requestActivate()
{
    Q_D(DShellSurface);

    d->requestActivate();
}

void DShellSurface::sendSignal(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

    // 以特殊名称的属性模拟信号发送，此属性不会记录在属性map中，且不能被get，只是借用属性的通知行为
    d->set_property(QStringLiteral(SIGNAL_PREFIX) + name, value);
}

DShellSurface::DShellSurface(DShellSurfacePrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}

QRect DShellSurface::geometry() const
{
    Q_D(const DShellSurface);

    if (!d->geometry.isValid()) {
        const_cast<DShellSurfacePrivate *>(d)->get_geometry();
    }

    return d->geometry;
}

QVariantMap DShellSurface::properties() const
{
    Q_D(const DShellSurface);
    return d->properies;
}

}
