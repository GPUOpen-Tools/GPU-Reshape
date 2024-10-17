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

using System.IO;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Services.Suspension;

namespace Studio.ViewModels.Setting
{
    public class FeaturesSettingViewModel : BaseSettingViewModel, INotifySuspension
    {
        /// <summary>
        /// Current texel addressing state
        /// </summary>
        [DataMember]
        public bool TexelAddressing
        {
            get => _texelAddressing;
            set => this.RaiseAndSetIfChanged(ref _texelAddressing, value);
        }
        
        public FeaturesSettingViewModel() : base("Features")
        {
            
        }

        public void Suspending()
        {
            // Ensure startup path exists
            if (System.IO.Path.GetDirectoryName(Path) is { } directory)
            {
                Directory.CreateDirectory(directory);
            }
            
            try
            {
                // Create environment view
                var view = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());

                // Write all global configs
                AppendGlobalConfig(view);
                
                // Write startup environment file for the global config
                using (var stream = new FileStream(Path, FileMode.Create, FileAccess.Write))
                {
                    stream.Write(view.Storage.Data.GetBuffer(), 0, (int)view.Storage.Data.Length);
                }
            }
            catch
            {
                Studio.Logging.Error("Failed to serialize global startup environment");
            }
        }

        /// <summary>
        /// Append all global settings
        /// </summary>
        private void AppendGlobalConfig(OrderedMessageView<ReadWriteMessageStream> view)
        {
            var texelMessage = view.Add<SetTexelAddressingMessage>();
            texelMessage.enabled = TexelAddressing ? 1 : 0;
        }
        
        /// <summary>
        /// Internal texel addressing state
        /// </summary>
        private bool _texelAddressing = true;
        
        /// <summary>
        /// Path for startup environment directives
        /// </summary>
        private static readonly string Path = System.IO.Path.Combine("Intermediate", "Settings", "GlobalEnvironment.env");
    }
}