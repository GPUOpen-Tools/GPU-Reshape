﻿// 
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

//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Studio.Resources {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "4.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    public class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        public static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("Studio.Resources.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        public static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Add a new application.
        /// </summary>
        public static string ApplicationList_Add_Tooltip {
            get {
                return ResourceManager.GetString("ApplicationList_Add_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Pause/resume all instrumentation.
        /// </summary>
        public static string BusMode_Toggle_Tooltip {
            get {
                return ResourceManager.GetString("BusMode_Toggle_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Pause validation reporting.
        /// </summary>
        public static string Detail_Resource_Pause {
            get {
                return ResourceManager.GetString("Detail_Resource_Pause", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Configure discoverability.
        /// </summary>
        public static string Discovery_DropDown_Tooltip {
            get {
                return ResourceManager.GetString("Discovery_DropDown_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Global discovery requires the installation of GPU Reshape services, which are added to the (OS) startup programs. Disabling global discovery will uninstall the services.
        ///
        ///Do you consent to this?.
        /// </summary>
        public static string Discovery_Warning_Content {
            get {
                return ResourceManager.GetString("Discovery_Warning_Content", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Service Installation.
        /// </summary>
        public static string Discovery_Warning_Title {
            get {
                return ResourceManager.GetString("Discovery_Warning_Title", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Remove this entry.
        /// </summary>
        public static string EditLabel_Close_Tooltip {
            get {
                return ResourceManager.GetString("EditLabel_Close_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Texel Addressing enables tracking initialization / concurrency events on a per-texel level, greatly improving accuracy. This incurs a heavy memory and performance overhead, disable to track on a per-resource level..
        /// </summary>
        public static string Features_TexelAddressing_Info {
            get {
                return ResourceManager.GetString("Features_TexelAddressing_Info", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Display help.
        /// </summary>
        public static string Help_Tooltip {
            get {
                return ResourceManager.GetString("Help_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Compiling {0} pipeline libraries....
        /// </summary>
        public static string Instrumentation_Stage_PipelineLibraries {
            get {
                return ResourceManager.GetString("Instrumentation_Stage_PipelineLibraries", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Compiling {0} pipelines....
        /// </summary>
        public static string Instrumentation_Stage_Pipelines {
            get {
                return ResourceManager.GetString("Instrumentation_Stage_Pipelines", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Compiling {0} shaders....
        /// </summary>
        public static string Instrumentation_Stage_Shaders {
            get {
                return ResourceManager.GetString("Instrumentation_Stage_Shaders", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Attach to all graphical devices the application creates, a workspace is automatically created per device.
        /// </summary>
        public static string Launch_AttachAllDevices {
            get {
                return ResourceManager.GetString("Launch_AttachAllDevices", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Hook into the entire process tree of an application, automatically creates process hierarchy.
        /// </summary>
        public static string Launch_CaptureChildProcesses {
            get {
                return ResourceManager.GetString("Launch_CaptureChildProcesses", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Enables detailed validation reporting, affects performance..
        /// </summary>
        public static string Launch_Detail {
            get {
                return ResourceManager.GetString("Launch_Detail", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Safe guarding prevents undefined behaviour from taking place due to erroneous operations. Affects performance..
        /// </summary>
        public static string Launch_SafeGuard {
            get {
                return ResourceManager.GetString("Launch_SafeGuard", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Open a folder.
        /// </summary>
        public static string Launch_SelectFolder_Tooltip {
            get {
                return ResourceManager.GetString("Launch_SelectFolder_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Enables use of synchronous recording, pipeline usage waits on compilation to finish. Required for certain features..
        /// </summary>
        public static string Launch_SynchronousRecording {
            get {
                return ResourceManager.GetString("Launch_SynchronousRecording", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Enables texel addressing, imposes large memory overhead.
        /// </summary>
        public static string Launch_TexelAddressing {
            get {
                return ResourceManager.GetString("Launch_TexelAddressing", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Clear all logging entries.
        /// </summary>
        public static string Log_Close_Tooltip {
            get {
                return ResourceManager.GetString("Log_Close_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Logging filter.
        /// </summary>
        public static string Log_Filter_Tooltip {
            get {
                return ResourceManager.GetString("Log_Filter_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Open attached report.
        /// </summary>
        public static string Log_Report_Tooltip {
            get {
                return ResourceManager.GetString("Log_Report_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Scroll to the bottom with new logging entries.
        /// </summary>
        public static string Log_ScrollToBottom_Tooltip {
            get {
                return ResourceManager.GetString("Log_ScrollToBottom_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Clear all validation messages.
        /// </summary>
        public static string MessageCollection_Clear_Tooltip {
            get {
                return ResourceManager.GetString("MessageCollection_Clear_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Filter validation messages.
        /// </summary>
        public static string MessageCollection_Filter_Tooltip {
            get {
                return ResourceManager.GetString("MessageCollection_Filter_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Network data read by this session.
        /// </summary>
        public static string NetworkStatus_Download_Tooltip {
            get {
                return ResourceManager.GetString("NetworkStatus_Download_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Network data written by this session.
        /// </summary>
        public static string NetworkStatus_Upload_Tooltip {
            get {
                return ResourceManager.GetString("NetworkStatus_Upload_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Add a new PDB search path.
        /// </summary>
        public static string PDBSetting_Add_Tooltip {
            get {
                return ResourceManager.GetString("PDBSetting_Add_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Refresh all pipeline entries.
        /// </summary>
        public static string PipelineTree_Refresh_Tooltip {
            get {
                return ResourceManager.GetString("PipelineTree_Refresh_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Show this validation error in IL.
        /// </summary>
        public static string Shader_Code_ShowInIL {
            get {
                return ResourceManager.GetString("Shader_Code_ShowInIL", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Show this validation error in source.
        /// </summary>
        public static string Shader_Code_ShowInSource {
            get {
                return ResourceManager.GetString("Shader_Code_ShowInSource", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Shader not compiled with debugging symbols, please ensure the shader is compiled with embedded debugging data..
        /// </summary>
        public static string Shader_NoDebugSymbols {
            get {
                return ResourceManager.GetString("Shader_NoDebugSymbols", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Shader not found in target application..
        /// </summary>
        public static string Shader_NotFound {
            get {
                return ResourceManager.GetString("Shader_NotFound", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Refresh all shader entries.
        /// </summary>
        public static string ShaderTree_Refresh_Tooltip {
            get {
                return ResourceManager.GetString("ShaderTree_Refresh_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Selected shader files.
        /// </summary>
        public static string Tool_Files {
            get {
                return ResourceManager.GetString("Tool_Files", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Workspace logs.
        /// </summary>
        public static string Tool_Log {
            get {
                return ResourceManager.GetString("Tool_Log", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All workspace pipelines.
        /// </summary>
        public static string Tool_Pipelines {
            get {
                return ResourceManager.GetString("Tool_Pipelines", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Instrumentation properties.
        /// </summary>
        public static string Tool_Properties {
            get {
                return ResourceManager.GetString("Tool_Properties", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All workspace shaders.
        /// </summary>
        public static string Tool_Shaders {
            get {
                return ResourceManager.GetString("Tool_Shaders", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All application workspaces.
        /// </summary>
        public static string Tool_Workspaces {
            get {
                return ResourceManager.GetString("Tool_Workspaces", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Collapse all items.
        /// </summary>
        public static string Tree_Collapse_Tooltip {
            get {
                return ResourceManager.GetString("Tree_Collapse_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Close all live workspaces.
        /// </summary>
        public static string Tree_DestroyAll_Tooltip {
            get {
                return ResourceManager.GetString("Tree_DestroyAll_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Expand all items.
        /// </summary>
        public static string Tree_Expand_Tooltip {
            get {
                return ResourceManager.GetString("Tree_Expand_Tooltip", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All configurations.
        /// </summary>
        public static string Workspace_Configuration_All_Description {
            get {
                return ResourceManager.GetString("Workspace_Configuration_All_Description", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All configurations, includes.
        /// </summary>
        public static string Workspace_Configuration_All_Header {
            get {
                return ResourceManager.GetString("Workspace_Configuration_All_Header", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to All Standard.
        /// </summary>
        public static string Workspace_Configuration_All_Name {
            get {
                return ResourceManager.GetString("Workspace_Configuration_All_Name", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Basic configuration, includes.
        /// </summary>
        public static string Workspace_Configuration_Basic_Header {
            get {
                return ResourceManager.GetString("Workspace_Configuration_Basic_Header", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Basic.
        /// </summary>
        public static string Workspace_Configuration_Basic_Name {
            get {
                return ResourceManager.GetString("Workspace_Configuration_Basic_Name", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Setup a custom workspace.
        /// </summary>
        public static string Workspace_Configuration_Custom_Description {
            get {
                return ResourceManager.GetString("Workspace_Configuration_Custom_Description", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Custom.
        /// </summary>
        public static string Workspace_Configuration_Custom_Name {
            get {
                return ResourceManager.GetString("Workspace_Configuration_Custom_Name", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Create a new workspace connection.
        /// </summary>
        public static string Workspace_Connect {
            get {
                return ResourceManager.GetString("Workspace_Connect", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Refresh the workspace.
        /// </summary>
        public static string Workspace_Refresh_Tooltip {
            get {
                return ResourceManager.GetString("Workspace_Refresh_Tooltip", resourceCulture);
            }
        }
    }
}
