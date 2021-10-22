#pragma once

#include "Pass.h"
#include <DiagnosticData.h>

namespace SPIRV 
{
	class InjectionPass : public Pass
	{
	public:
		InjectionPass(ShaderState* state, const char* name) : Pass(state, name)
		{
		}

		/// Overrides
		Status Process() override;
		spvtools::opt::IRContext::Analysis GetPreservedAnalyses() override;

		/**
		 * Visit a block for injection
		 * @param[in] block
		 */
		virtual bool Visit(spvtools::opt::BasicBlock* block) = 0;

	public:
		/**
		 * Find or create a source extract from an instruction
		 * @param[in] block the parent block
		 * @param[in] iit the instruction iterator within <block>
		 */
		uint32_t FindSourceExtractGUID(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit);

		/**
		 * Split a basic block into two
		 *   block: [A B C D E F G]
		 *                   ^-iit
		 *   block: [A B C D]
		 *   split:         [E F G]
		 * @param[in] block the block to split
		 * @param[in] iit the split point, invalidated after the split
		 * @param[in] local if true all future injections will ignore this block
		 */
		spvtools::opt::BasicBlock* SplitBasicBlock(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit, bool local);

		/**
		 * Allocate a new block
		 * @param[in] after the insertion point, the new block will be inserted after this
		 * @param[in] local if true all future injections will ignore this block
		 */
		spvtools::opt::BasicBlock* AllocBlock(spvtools::opt::BasicBlock* after, bool local);

		/**
		 * Allocate a new resulted instruction
		 * @param[in] op the spirv op
		 * @param[in] ty_id the result type id, 0 indicating no type id
		 * @param[in] operands the instruction operands
		 */
		std::unique_ptr<spvtools::opt::Instruction> AllocInstr(SpvOp op, uint32_t ty_id, const spvtools::opt::Instruction::OperandList& operands = {});

		/**
		 * Find the type of a declaration
		 * @param[in] id the source instruction id
		 */
		spvtools::opt::Instruction* FindDeclarationType(uint32_t id);

		/**
		 * Find the declaration
		 * @param[in] id the source instruction id
		 */
		spvtools::opt::Instruction* FindDeclaration(uint32_t id);

		/**
		 * Mark an instruction as injected
		 * @param[in] id the source instruction id
		 */
		spvtools::opt::Instruction* Track(spvtools::opt::Instruction* instr)
		{
			MarkAsInjected(instr);
			return instr;
		}

		/**
		 * Mark an instruction as injected
		 * @param[in] id the source instruction id
		 */
		void MarkAsInjected(const spvtools::opt::Instruction* instr);

		/**
		 * Check if an instruction is injected
		 * @param[in] id the source instruction id
		 */
		bool IsInjectedInstruction(const spvtools::opt::Instruction* instr);

		/**
		 * Get the descriptor set binding information from a given result id
		 * @param id the result id to be traversed
		 * @param[out] set the written set index
		 * @param[out] binding the written binding index
		 * @return true if the binding information was found
		 */
		bool GetDescriptorBinds(uint32_t id, uint32_t* set, uint32_t* binding);

	public:
		/**
		 * Get the registry descriptor state
		 * @param[in] set_id the originating set id
		 * @param[in] descriptor_uid the registered descriptor uid
		 */
		DescriptorState* GetRegistryDescriptor(uint32_t set_id, uint16_t descriptor_uid);

		/**
		 * Load a push constant value
		 * @param[in] builder the instruction builder helper
		 * @param[in] pc_uid the registered push constant uid
		 */
		uint32_t LoadPushConstant(spvtools::opt::InstructionBuilder& builder, uint16_t pc_uid);

	public:
		/**
		 * Composite a static message
		 * @param[in] builder the instruction builder helper
		 * @param[in] data the static message data
		 */
		uint32_t CompositeStaticMessage(spvtools::opt::InstructionBuilder& builder, SDiagnosticMessageData data);

		/**
		 * Composite a dynamic (variable) message
		 * ! Inputs must not be bit-shifted, that is done internally
		 * @param[in] builder the instruction builder helper
		 * @param[in] type_id the type result id
		 * @param[in] message_id the message result id
		 */
		uint32_t CompositeDynamicMessage(spvtools::opt::InstructionBuilder& builder, uint32_t type_id, uint32_t message_id);

		/**
		 * Increase the global message counter
		 * @param[in] builder the instruction builder helper
		 * @param[in] count the number of messages to be exported
		 */
		uint32_t PushMessages(spvtools::opt::InstructionBuilder& builder, uint32_t count);

		/**
		 * Export a message to the current diagnostics allocation
		 * @param[in] builder the instruction builder helper
		 * @param[in] composite_id the id of the message composite to export
		 */
		void ExportMessage(spvtools::opt::InstructionBuilder& builder, uint32_t id, uint32_t composite_id);

		/**
		 * Export a message to the current diagnostics allocation
		 * @param[in] builder the instruction builder helper
		 * @param[in] composite_id the id of the message composite to export
		 */
		void ExportMessage(spvtools::opt::InstructionBuilder& builder, uint32_t composite_id);

	private:
		// A potential source level candidate
		struct SourceCandidate
		{
			spvtools::opt::Instruction* m_Instruction;
			uint32_t m_LineOffset = 0;
		};

		/**
		 * Find the source candidate of an instruction
		 * @param[in] block the parent block
		 * @param[in] iit the instruction iterator within <block>
		 */
		SourceCandidate FindCandidate(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit);

		/**
		 * Find the source candidate of an inlined [block] instruction
		 * @param[in] block the parent block
		 * @param[in] iit the instruction iterator within <block>
		 */
		SourceCandidate FindInlinedSource(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit);

		/**
		 * Find the source candidate of a callee instruction
		 * @param[in] block the parent block
		 * @param[in] iit the instruction iterator within <block>
		 */
		SourceCandidate FindCalleeSource(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit);
	};
}
