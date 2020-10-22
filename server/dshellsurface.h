#ifndef DSHELLSURFACE_H
#define DSHELLSURFACE_H

#include <QtWaylandCompositor/QWaylandCompositorExtension>
#include <QtWaylandCompositor/QWaylandShellSurfaceTemplate>

QT_BEGIN_NAMESPACE
class QWaylandSurface;
class QWaylandResource;
class QWaylandSurfaceRole;
QT_END_NAMESPACE

struct wl_display;
struct wl_resource;

namespace DWaylandServer {

class DShellSurface;
class DShellSurfaceManagerPrivate;
class DShellSurfaceManager : public QWaylandCompositorExtensionTemplate<DShellSurfaceManager>
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DShellSurfaceManager)
public:
    explicit DShellSurfaceManager(struct ::wl_display *display);
    explicit DShellSurfaceManager(QWaylandCompositor *compositor = nullptr);
    ~DShellSurfaceManager();

    static const struct wl_interface *interface();
    static QByteArray interfaceName();

Q_SIGNALS:
    void surfaceCreated(DShellSurface *surface);

private:
    void initialize() override;

    QScopedPointer<DShellSurfaceManagerPrivate> d_ptr;
};

class DShellSurfacePrivate;
class DShellSurface : public QWaylandShellSurfaceTemplate<DShellSurface>
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DShellSurface)
public:
    explicit DShellSurface(DShellSurfaceManager *manager, wl_resource *resource, wl_resource *surfaceResource, uint id);
    ~DShellSurface();

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
#if QT_CONFIG(wayland_compositor_quick)
    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override { return nullptr; }
#endif
#else
#ifdef QT_WAYLAND_COMPOSITOR_QUICK
    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override { return nullptr; }
#endif
#endif

    struct ::wl_resource *resource() const;
    wl_resource *surfaceResource() const;
    uint id() const;
    void setGeometry(const QRect &rect);
    QVariant property(const QString &name) const;
    void setProperty(const QString &name, const QVariant &value);

    static const struct wl_interface *interface();
    static QByteArray interfaceName();
    static QWaylandSurfaceRole *role();
    static DShellSurface *fromResource(struct ::wl_resource *resource);

public Q_SLOTS:
    void sendSignal(const QString &name, const QVariant &value);

Q_SIGNALS:
    void surfaceChanged();
    void idChanged();
    void propertyChanged(const QString &name, const QVariant &value);
    void notify(const QString &name, const QVariant &value);
    void activationRequested();
    void surfaceDestroyed();

private:
    void initialize() override;

    QScopedPointer<DShellSurfacePrivate> d_ptr;
};

} // DWaylandServer

#endif // DSHELLSURFACE_H
