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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Message.CLR;

public static class Test
{
    public static void Main(string[] args)
    {
        // Run tests on all schema types
        StaticStreamTest();
        ChunkedStreamTest();
        DynamicStreamTest();
        OrderedStreamTest();
    }

    // Shared message type
    private struct FixedMessage : IChunkedMessage
    {
        public ByteSpan Memory
        {
            set => _memory = value;
        }
        
        // Chunked size
        public uint RuntimeByteSize => 4u;
        
        // Dummy value
        public UInt32 Value
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<UInt32>(_memory.AsRefSpan());

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => MemoryMarshal.Write<UInt32>(_memory.AsRefSpan(), ref value);
        }

        // Default allocation info
        public struct AllocationInfo : IMessageAllocationRequest
        {
            public void Patch(IMessage message)
            {
                
            }

            public void Default(IMessage message)
            {
                
            }

            public ulong ByteSize => 4u;
            
            public uint ID => 0;
        }
        
        // Get default request
        public IMessageAllocationRequest DefaultRequest()
        {
            return new AllocationInfo();
        }
        
        private ByteSpan _memory;
    }

    /// <summary>
    /// Benchmark static schemas
    /// </summary>
    private static void StaticStreamTest()
    {
        var stream = new ReadWriteMessageStream();

        // Set schema
        stream.GetOrSetSchema(new MessageSchema()
        {
            type = MessageSchemaType.Static,
            id = 0
        });

        // Allocate messages
        {
            var view = new StaticMessageView<FixedMessage, ReadWriteMessageStream>(stream);
            
            for (uint i = 0; i < MessageCount; i++)
            {
                FixedMessage message = view.Add();
                message.Value = 42;
            }
        }
        
        // Create typed view
        var readOnlyView = new StaticMessageView<FixedMessage>(stream.ToReadOnly());

        Console.WriteLine("Static");

        // Typed unsafe enumeration
        Benchmark("Typed Unsafe", () =>
        {
            var enumerator = readOnlyView.GetUnsafeEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed guarded enumeration
        Benchmark("Typed Guarded", () =>
        {
            var enumerator = readOnlyView.GetGuardedEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed foreach enumeration
        Benchmark("Typed Foreach", () =>
        {
            foreach (FixedMessage message in readOnlyView)
            {
                if (message.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Untyped (IEnumerator) benchmarks
        UntypedBenchmarks(readOnlyView, (int)stream.Count);
    }

    private static void ChunkedStreamTest()
    {
        var stream = new ReadWriteMessageStream();

        // Set schema
        stream.GetOrSetSchema(new MessageSchema()
        {
            type = MessageSchemaType.Chunked,
            id = 0
        });

        // Allocate messages
        for (uint i = 0; i < MessageCount; i++)
        {
            // Allocate message
            FixedMessage message = new FixedMessage();
            message.Memory = stream.Allocate((int)message.RuntimeByteSize);
            message.Value = 42;
        }

        // Create typed view
        var readOnlyView = new ChunkedMessageView<FixedMessage>(stream.ToReadOnly());

        Console.WriteLine("Chunked");
        
        // Typed unsafe enumeration
        Benchmark("Typed Unsafe", () =>
        {
            var enumerator = readOnlyView.GetUnsafeEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed guarded enumeration
        Benchmark("Typed Guarded", () =>
        {
            var enumerator = readOnlyView.GetGuardedEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed foreach enumeration
        Benchmark("Typed Foreach", () =>
        {
            foreach (FixedMessage message in readOnlyView)
            {
                if (message.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Untyped (IEnumerator) benchmarks
        UntypedBenchmarks(readOnlyView, (int)stream.Count);
    }

    private static void DynamicStreamTest()
    {
        var stream = new ReadWriteMessageStream();

        // Set schema
        stream.GetOrSetSchema(new MessageSchema()
        {
            type = MessageSchemaType.Dynamic,
            id = 0
        });

        // Allocate messages
        {
            var view = new DynamicMessageView<FixedMessage, ReadWriteMessageStream>(stream);
            
            for (uint i = 0; i < MessageCount; i++)
            {
                FixedMessage message = view.Add();
                message.Value = 42;
            }
        }

        // Create typed view
        var readOnlyView = new DynamicMessageView<FixedMessage>(stream.ToReadOnly());

        Console.WriteLine("Dynamic");
        
        // Typed unsafe enumeration
        Benchmark("Typed Unsafe", () =>
        {
            var enumerator = readOnlyView.GetUnsafeEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed guarded enumeration
        Benchmark("Typed Guarded", () =>
        {
            var enumerator = readOnlyView.GetGuardedEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Typed foreach enumeration
        Benchmark("Typed Foreach", () =>
        {
            foreach (FixedMessage message in readOnlyView)
            {
                if (message.Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        });

        // Untyped (IEnumerator) benchmarks
        UntypedBenchmarks(readOnlyView, (int)stream.Count);
    }

    private static void OrderedStreamTest()
    {
        var stream = new ReadWriteMessageStream();

        // Set schema
        stream.GetOrSetSchema(new MessageSchema()
        {
            type = MessageSchemaType.Ordered,
            id = 0
        });

        // Allocate messages
        {
            var view = new OrderedMessageView<ReadWriteMessageStream>(stream);
            
            for (uint i = 0; i < MessageCount; i++)
            {
                FixedMessage message = view.Add<FixedMessage>();
                message.Value = 42;
            }
        }

        // Create typed view
        var readOnlyView = new OrderedMessageView(stream.ToReadOnly());

        Console.WriteLine("Ordered");
        
        // Typed unsafe enumeration
        Benchmark("Typed Unsafe", () =>
        {
            var enumerator = readOnlyView.GetUnsafeEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.ID != 0 || enumerator.Current.Get<FixedMessage>().Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        }, false);

        // Typed guarded enumeration
        Benchmark("Typed Guarded", () =>
        {
            var enumerator = readOnlyView.GetGuardedEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.ID != 0 || enumerator.Current.Get<FixedMessage>().Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        }, false);

        // Typed foreach enumeration
        Benchmark("Typed Foreach", () =>
        {
            foreach (OrderedMessage message in readOnlyView)
            {
                if (message.ID != 0 || message.Get<FixedMessage>().Value != 42)
                {
                    return 0;
                }
            }

            return (int)stream.Count;
        }, false);

        UntypedBenchmarks(readOnlyView, (int)stream.Count);
    }

    private static void UntypedBenchmarks(IEnumerable<FixedMessage> enumerable, int count)
    {
        // Untyped enumeration
        Benchmark("Enumerable Enumerator", () =>
        {
            var enumerator = enumerable.GetEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.Value != 42)
                {
                    return 0;
                }
            }

            return count;
        });

        // Untyped foreach
        Benchmark("Enumerable Foreach", () =>
        {
            foreach (FixedMessage message in enumerable)
            {
                if (message.Value != 42)
                {
                    return 0;
                }
            }

            return count;
        });
    }

    private static void UntypedBenchmarks(IEnumerable<OrderedMessage> enumerable, int count)
    {
        // Untyped enumeration
        Benchmark("Enumerable Enumerator", () =>
        {
            var enumerator = enumerable.GetEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (enumerator.Current.ID != 0 || enumerator.Current.Get<FixedMessage>().Value != 42)
                {
                    return 0;
                }
            }

            return count;
        }, false);

        // Untyped foreach
        Benchmark("Enumerable Foreach", () =>
        {
            foreach (OrderedMessage message in enumerable)
            {
                if (message.ID != 0 || message.Get<FixedMessage>().Value != 42)
                {
                    return 0;
                }
            }

            return count;
        }, false);
    }

    /// <summary>
    /// Run a benchmark
    /// </summary>
    private static void Benchmark(string name, Func<int> action, bool disableGC = true)
    {
        // Requested no GC?
        if (disableGC)
        {
            GC.Collect(2, GCCollectionMode.Forced);
            GC.TryStartNoGCRegion((int)2e+8);   
        }
        
        // Create watch
        Stopwatch watch = Stopwatch.StartNew();
        
        int count = 0;

        // Run warmup iterations
        for (int iteration = 0; iteration < IterationWarmupEnd; iteration++)
        {
            count += action();
        }
        
        // Start timer
        watch.Restart();

        // Run iterations
        for (int iteration = IterationWarmupEnd; iteration < IterationCount; iteration++)
        {
            count += action();
        }

        // Validate count (mainly to avoid DCE)
        if (count != IterationCount * MessageCount)
        {
            return;
        }
            
        // Get elapsed time
        double duration = watch.ElapsedMilliseconds / (double)(IterationCount - IterationWarmupEnd);
        
        // Resume GC if requested
        if (disableGC)
        {
            GC.EndNoGCRegion();
        }

        Console.WriteLine($"\t{name}: {duration} ms");
    }

    // Number of warmup iterations, does not count for benchmarking
    private static int IterationWarmupEnd = 10;

    // Number of iterations
    private static int IterationCount = 50;

    // Number of messages per iteration
    private static int MessageCount = 10000000;
}
