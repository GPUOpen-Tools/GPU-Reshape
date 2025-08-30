using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseVirtualObservableCollection : IReadOnlyList<LooseItemViewModel>, IList, INotifyCollectionChanged, INotifyPropertyChanged
{
    /// <summary>
    /// Generic enumerator
    /// </summary>
    public class Enumerator : IEnumerator<LooseItemViewModel>
    {
        /// <summary>
        /// Parent container
        /// </summary>
        public LooseVirtualObservableCollection Container;

        /// <summary>
        /// Get the current item
        /// </summary>
        public LooseItemViewModel Current
        {
            get
            {
                return new LooseItemViewModel(Container.BreakpointDisplayViewModel, _dwordOffset)
                {
                    Index = _index
                };
            }
        }

        object IEnumerator.Current => Current;

        /// <summary>
        /// Next item
        /// </summary>
        public bool MoveNext()
        {
            DebugBreakpointStreamMessage.FlatInfo flatInfo = Container.BreakpointDisplayViewModel.FlatInfo;
            _dwordOffset += LooseBreakpointHeader.DWordCount + flatInfo.dataDWordStride;
            _index++;
            return _dwordOffset + LooseBreakpointHeader.DWordCount + flatInfo.dataDWordStride <= Container.BreakpointDisplayViewModel.DWords.Length;
        }

        /// <summary>
        /// Reset this enumerator
        /// </summary>
        public void Reset()
        {
            _index = 0;
            _dwordOffset = 0;
        }

        /// <summary>
        /// Dummy disposal
        /// </summary>
        public void Dispose()
        {
            
        }

        /// <summary>
        /// Current item index
        /// </summary>
        private int _index = 0;
        
        /// <summary>
        /// Data wise dword offset
        /// </summary>
        private uint _dwordOffset = 0;
    }

    /// <summary>
    /// The owning display view model
    /// </summary>
    public required LooseBreakpointDisplayViewModel BreakpointDisplayViewModel { get; set; }

    /// <summary>
    /// Always synchronized
    /// </summary>
    public bool IsSynchronized => true;

    /// <summary>
    /// Always fixed size
    /// </summary>
    public bool IsFixedSize => true;

    /// <summary>
    /// Cannot be modified
    /// </summary>
    public bool IsReadOnly => true;

    /// <summary>
    /// Get the number of items
    /// </summary>
    public int Count
    {
        get
        {
            return (int)BreakpointDisplayViewModel.FlatInfo.dataDynamicCounter;
        }
    }

    /// <summary>
    /// Sync with the breakpoint by default
    /// </summary>
    public object SyncRoot => BreakpointDisplayViewModel;

    /// <summary>
    /// Array accessor
    /// </summary>
    public LooseItemViewModel? this[int index]
    {
        get
        {
            uint dwordOffset = (uint)((LooseBreakpointHeader.DWordCount + BreakpointDisplayViewModel.FlatInfo.dataDWordStride) * index);
            
            // Guard against expected bounds
            if (dwordOffset + LooseBreakpointHeader.DWordCount + BreakpointDisplayViewModel.FlatInfo.dataDWordStride > BreakpointDisplayViewModel.DWords.Length)
            {
                return null;
            }

            // Construct at offset
            return new LooseItemViewModel(BreakpointDisplayViewModel, dwordOffset)
            {
                Index = index
            };
        }
    }

    /// <summary>
    /// Generic accessor
    /// </summary>
    object? IList.this[int index]
    {
        get
        {
            return this[index];
        }
        set
        {
            throw new ReadOnlyException();
        }
    }

    /// <summary>
    /// Unused, collection changed events
    /// </summary>
    public event NotifyCollectionChangedEventHandler? CollectionChanged;
    
    /// <summary>
    /// Unused, property changed events
    /// </summary>
    public event PropertyChangedEventHandler? PropertyChanged;
    
    /// <summary>
    /// Stub
    /// </summary>
    public int Add(object? value)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public void Clear()
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public bool Contains(object? value)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public int IndexOf(object? value)
    {
        return (value as LooseItemViewModel)?.Index ?? -1;
    }

    /// <summary>
    /// Stub
    /// </summary>
    public void Insert(int index, object? value)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public void Remove(object? value)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public void RemoveAt(int index)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Stub
    /// </summary>
    public void CopyTo(Array array, int index)
    {
        throw new ReadOnlyException();
    }

    /// <summary>
    /// Get non-virtual enumerator
    /// </summary>
    public IEnumerator<LooseItemViewModel> GetEnumerator()
    {
        return new Enumerator()
        {
            Container = this
        };
    }

    /// <summary>
    /// Get non-virtual enumerator
    /// </summary>
    IEnumerator IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }
}
