// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

namespace Studio.Models.Workspace.Objects
{
    public struct Resource
    {
        /// <summary>
        /// Physical unique identifier
        /// </summary>
        public uint PUID { get; set; }

        /// <summary>
        /// Version of this resource
        /// </summary>
        public uint Version { get; set; }

        /// <summary>
        /// Lookup key
        /// </summary>
        public ulong Key => (ulong)PUID | ((ulong)Version << 32);

        /// <summary>
        /// Name of this resource
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Width of this resource
        /// </summary>
        public uint Width { get; set; }

        /// <summary>
        /// Height of this resource
        /// </summary>
        public uint Height { get; set; }

        /// <summary>
        /// Depth of this resource
        /// </summary>
        public uint Depth { get; set; }
        
        /// <summary>
        /// Native format of this resource
        /// </summary>
        public string Format { get; set; }
        
        /// <summary>
        /// Is this resource a stand-in for a future streamed resource?
        /// </summary>
        public bool IsUnknown { get; set; }
    }
}