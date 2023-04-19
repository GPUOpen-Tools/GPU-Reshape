using System;
using System.Reactive;
using System.Reactive.Subjects;
using Bridge.CLR;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Workspace
{
    public class VirtualConnectionViewModel : ReactiveObject, IConnectionViewModel
    {
        /// <summary>
        /// Invoked during connection
        /// </summary>
        public ISubject<Unit> Connected { get; }

        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        public ISubject<Unit> Refused { get; }

        /// <summary>
        /// Local time on the remote endpoint
        /// </summary>
        public DateTime LocalTime
        {
            get => _localTime;
            set => this.RaiseAndSetIfChanged(ref _localTime, value);
        }

        /// <summary>
        /// Endpoint application info
        /// </summary>
        public ApplicationInfo? Application { get; set; }

        /// <summary>
        /// Underlying bridge of this connection
        /// </summary>
        public IBridge? Bridge => null;

        /// <summary>
        /// Constructor
        /// </summary>
        public VirtualConnectionViewModel()
        {
            Connected = new Subject<Unit>();
            Refused = new Subject<Unit>();
        }

        /// <summary>
        /// Get the shared bus
        /// </summary>
        /// <returns></returns>
        public OrderedMessageView<ReadWriteMessageStream> GetSharedBus()
        {
            return _sharedStream;
        }

        /// <summary>
        /// Commit the bus
        /// </summary>
        public void Commit()
        {
            _sharedStream = new(new ReadWriteMessageStream());
        }
                
        /// <summary>
        /// Dummy stream
        /// </summary>
        private OrderedMessageView<ReadWriteMessageStream> _sharedStream = new(new ReadWriteMessageStream());

        /// <summary>
        /// Internal local time
        /// </summary>
        private DateTime _localTime;
    }
}