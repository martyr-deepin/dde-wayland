#include "dshellsurface.h"

#include <qwayland-dde-shell.h>
#include <QWindow>
#include <QGuiApplication>
#include <QWaylandClientExtensionTemplate>
#include <QPlatformSurfaceEvent>
#include <qpa/qplatformnativeinterface.h>

static inline struct ::wl_surface *getWlSurface(QWindow *window)
{
    void *surf = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window);
    return static_cast<struct ::wl_surface *>(surf);
}

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

    DShellSurfaceManager *q_ptr;
    Q_DECLARE_PUBLIC(DShellSurfaceManager)
};

class DShellSurfacePrivate : public QWaylandClientExtensionTemplate<DShellSurfacePrivate>, public QtWayland::dde_shell_surface
{
public:
    DShellSurfacePrivate(struct ::dde_shell_surface *object)
        : QWaylandClientExtensionTemplate(1)
        , QtWayland::dde_shell_surface(object)
    {

    }

    virtual void dde_shell_surface_geometry(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        QRect geometry(x, y, w, h);

        if (geometry == this->geometry)
            return;

        this->geometry = geometry;

        Q_Q(DShellSurface);
        Q_EMIT q->geometryChanged(geometry);
    }
    virtual void dde_shell_surface_property(const QString &name, wl_array *value)
    {
        const QByteArray data(static_cast<char *>(value->data), value->size * 4);
        QDataStream ds(data);
        QVariant vv;
        ds >> vv;
        properies[name] = vv;

        Q_Q(DShellSurface);
        Q_EMIT q->propertyChanged(name, vv);
    }

    void set_property(const QString &name, const QVariant &value)
    {
        QByteArray data;
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << value;
        QtWayland::dde_shell_surface::set_property(name, data);
    }

    DShellSurface *q_ptr;
    QRect geometry;
    QVariantMap properies;

    Q_DECLARE_PUBLIC(DShellSurface)
};

DShellSurfaceManager::DShellSurfaceManager()
    : d_ptr(new DShellSurfaceManagerPrivate(this))
{

}

DShellSurfaceManager::~DShellSurfaceManager()
{

}

DShellSurface *DShellSurfaceManager::ensureShellSurface(wl_surface *surface)
{
    Q_D(DShellSurfaceManager);
    auto dde_surface = d->get_surface(surface);
    auto s = new DShellSurface(*new DShellSurfacePrivate(dde_surface));
    Q_EMIT surfaceCreated(s);

    return s;
}

DShellSurface *DShellSurfaceManager::registerWindow(QWindow *window)
{
    Q_D(DShellSurfaceManager);

    if (!d->isActive()) {
        connect(d, &DShellSurfaceManagerPrivate::activeChanged, this, [this, d, window] {
            Q_ASSERT_X(d->isActive(), "DShellSurfaceManager::registerWindow", "The surface manager is not active.");
            registerWindow(window);
        });

        return nullptr;
    }

    if (window->handle()) {
        return ensureShellSurface(getWlSurface(window));
    }

    // watch the window surface created
    d->installEventFilter(this);

    return nullptr;
}

DShellSurface::~DShellSurface()
{
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

void DShellSurface::setProperty(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

    d->properies[name] = value;
    d->set_property(name, value);
}

DShellSurface::DShellSurface(DShellSurfacePrivate &dd)
    : d_ptr(&dd)
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
