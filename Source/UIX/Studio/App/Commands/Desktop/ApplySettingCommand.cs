// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System.CommandLine;
using System.CommandLine.Invocation;
using System.Threading.Tasks;
using Studio.App.Commands.Desktop;
using Studio.Services;
using Studio.ViewModels.Setting;

namespace Studio.App.Commands;

public class ApplySettingCommand : IBaseCommand
{
    /// <summary>
    /// Create the apply setting command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("apply-setting").Make(new ApplySettingCommand(), new Option[]
        {
            JsonPath
        });
    }

    /// <summary>
    /// Command invocation
    /// </summary>
    public Task<int> InvokeAsync(InvocationContext context)
    {
        _context = context;
        
        // Try to load settings
        if (ApplySettingConfig.DeserializeFile(context.ParseResult.GetValueForOption(JsonPath)!) is not {} userSettings)
        {
            Logging.Error("Failed to load settings json");
            return Task.FromResult(1);
        }
        
        // Get settings
        if (ServiceRegistry.Get<ISettingsService>() is not {} settings)
        {
            Logging.Error("Failed to find settings");
            return Task.FromResult(1);
        }

        // Must have app-list
        if (settings.ViewModel.GetItem<ApplicationListSettingViewModel>() is not { } listSettings)
        {
            Logging.Error("Failed to find application setting list");
            return Task.FromResult(1);
        }
        
        // Try to find existing application with matching process
        var appSettings = listSettings.GetFirstItem<ApplicationSettingViewModel>(x => x.ApplicationName == userSettings.ApplicationName);
        if (appSettings == null)
        {
            // None found, create a new one
            appSettings = new ApplicationSettingViewModel()
            {
                ApplicationName = userSettings.ApplicationName
            };
                    
            // Add to apps
            listSettings.Items.Add(appSettings);
        }
        
        // Setup pdb settings
        var pdbSettings = appSettings.GetItemOrAdd<PDBSettingViewModel>();
        pdbSettings.SearchInSubFolders = userSettings.Symbol.IncludeSubDirectories;

        // Setup source settings
        var sourceSettings = appSettings.GetItemOrAdd<SourceSettingViewModel>();
        sourceSettings.IndexSubFolders = userSettings.Source.IncludeSubDirectories;

        // Add all symbol paths
        foreach (string path in userSettings.Symbol.Paths)
        {
            if (!pdbSettings.SearchDirectories.Contains(path))
            {
                pdbSettings.SearchDirectories.Add(path);
            }
        }
        
        // Add all source paths
        foreach (string path in userSettings.Source.Paths)
        {
            if (!sourceSettings.SourceDirectories.Contains(path))
            {
                sourceSettings.SourceDirectories.Add(path);
            }
        }
        
        // OK
        return Task.FromResult(0);
    }

    /// <summary>
    /// Json path
    /// </summary>
    private static readonly Option<string> JsonPath = new("-json", "The setting json file")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Internal context
    /// </summary>
    private InvocationContext _context;
}