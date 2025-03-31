using Studio.Plugin;
using Studio.Services;
using Studio.Utils;

namespace Studio;

public class ServiceProvider
{
    /// <summary>
    /// Install all services
    /// </summary>
    public void Install()
    {
        // Create shared registry
        ServiceRegistry.Install<DefaultServiceRegistry>();
            
        // Attempt to find all plugins of relevance
        _pluginList = _pluginResolver.FindPlugins("uix", PluginResolveFlag.ContinueOnFailure);
            
        // Cold suspension service
        ServiceRegistry.Add<ISuspensionService>(new SuspensionService(System.IO.Path.Combine(PathUtils.BaseDirectory, "Intermediate", "Settings", "Suspension.json")));
            
        // Locator
        ServiceRegistry.Add<ILocatorService>(new LocatorService());
            
        // Logging host
        ServiceRegistry.Add<ILoggingService>(new LoggingService());

        // Hosts all menu objects
        ServiceRegistry.Add<IWindowService>(new WindowService());

        // Hosts all live workspaces
        ServiceRegistry.Add<IWorkspaceService>(new WorkspaceService());
            
        // Provides general network diagnostics
        ServiceRegistry.Add(new NetworkDiagnosticService());

        // Initiates the host resolver if not already up and running
        ServiceRegistry.Add<IHostResolverService>(new HostResolverService());
            
        // Local discoverability
        ServiceRegistry.Add<IBackendDiscoveryService>(new BackendDiscoveryService());

        // Hosts all status objects
        ServiceRegistry.Add<IStatusService>(new StatusService());

        // Hosts all context objects
        ServiceRegistry.Add<IContextMenuService>(new ContextMenuService());

        // Hosts all menu objects
        ServiceRegistry.Add<IMenuService>(new MenuService());

        // Hosts all settings objects
        ServiceRegistry.Add<ISettingsService>(new SettingsService());
    }

    /// <summary>
    /// Install all service plugins
    /// </summary>
    public void InstallPlugins()
    {
        // Install all plugins
        if (_pluginList != null)
        {
            _pluginResolver.InstallPlugins(_pluginList, PluginResolveFlag.ContinueOnFailure);
        }
    }

    /// <summary>
    /// Shared plugin resolver
    /// </summary>
    private PluginResolver _pluginResolver = new();

    /// <summary>
    /// Resolved plugin list
    /// </summary>
    private PluginList? _pluginList;
}