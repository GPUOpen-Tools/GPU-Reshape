using System;
using Message.CLR;

namespace Runtime.Models.Objects
{
    public class InstrumentationState
    {
        /// <summary>
        /// Instrumentation bit mask, see feature infos
        /// </summary>
        public UInt64 FeatureBitMask { get; set; }
        
        /// <summary>
        /// Instrumentation specialization stream
        /// </summary>
        public OrderedMessageView<ReadWriteMessageStream> SpecializationStream { get; set; }
    }
}
