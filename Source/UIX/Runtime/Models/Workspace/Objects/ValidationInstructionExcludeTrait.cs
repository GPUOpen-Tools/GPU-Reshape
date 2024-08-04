using System.Linq;
using Studio.Models.IL;

namespace Studio.Models.Workspace.Objects
{
    public class ValidationInstructionExcludeTrait : IValidationTraits
    {
        /// <summary>
        /// All excluded op codes from detailed information
        /// </summary>
        public OpCode[] ExcludedOps = System.Array.Empty<OpCode>();
        
        /// <summary>
        /// Check if a validation object can produce detailed instrumentation data for a given location
        /// </summary>
        /// <param name="program">program to check in</param>
        /// <param name="location">location within program</param>
        /// <returns>true if detailed data</returns>
        public bool ProducesDetailDataFor(Program program, ShaderLocation location)
        {
            // Try to find instruction, if this failed, for whatever reason, just presume true
            if (program.GetInstruction(location) is not { } instr)
            {
                return true;
            }

            // Check if the op code is known no detailed
            return !ExcludedOps.Contains(instr.OpCode);
        }
    }
}