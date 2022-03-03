#include <catch2/catch.hpp>

// Backend
#include <Backend/IL/Emitter.h>

TEST_CASE("Backend.IL.BasicBlock") {
    Allocators allocators;

    IL::Program program(allocators, 0x0);

    IL::IdentifierMap& map = program.GetIdentifierMap();

    IL::Function* fn = program.GetFunctionList().AllocFunction(map.AllocID());

    IL::BasicBlock* bb = fn->GetBasicBlocks().AllocBlock(map.AllocID());

    IL::LiteralInstruction a;
    a.opCode = IL::OpCode::Literal;
    a.result = map.AllocID();
    a.type = IL::LiteralType::Int;
    a.bitWidth = 8;
    a.value.integral = 1;
    auto aRef = bb->Append(a);

    IL::LiteralInstruction b;
    b.opCode = IL::OpCode::Literal;
    b.result = map.AllocID();
    b.type = IL::LiteralType::Int;
    b.bitWidth = 8;
    b.value.integral = 1;
    IL::InstructionRef<IL::LiteralInstruction> bRef = bb->Append(b);

    IL::AddInstruction add;
    add.opCode = IL::OpCode::Add;
    add.result = map.AllocID();
    add.lhs = a.result;
    add.rhs = b.result;
    auto addRef = bb->Append(add);

    REQUIRE(aRef->value.integral == 1);
    REQUIRE(bRef.As<IL::LiteralInstruction>()->value.integral == 1);

    a.value.integral = 5;
    bb->Replace(aRef, a);

    REQUIRE(aRef.As<IL::LiteralInstruction>()->value.integral == 5);

    REQUIRE(addRef->lhs == aRef->result);
    REQUIRE(addRef->rhs == bRef->result);
}
