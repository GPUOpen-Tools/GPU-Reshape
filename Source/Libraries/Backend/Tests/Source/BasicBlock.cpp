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
