#include "PatchGen.h"

#include "Equality.h"
#include "cangjie/CHIR/Checker/CHIRChecker.h"
#include "cangjie/CHIR/Serializer/CHIRSerializer.h"
#include "cangjie/CHIR/Utils/Utils.h"

#include <iostream>

using namespace Cangjie::CHIR;

namespace PatchGenerator {

PatchGen::PatchGen(std::string fileName, Package* source, Package* patched, CHIRBuilder* builder)
    : fileName(std::move(fileName)),
      sourcePackage(source),
      patchedPackage(patched),
      builder(builder),
      guardVarsInitializer(findGuardVarsInitializer(patched)),
      guardClass(findGuardClass(patched)),
      guardVar(findGuardVar(patched))
{
    error((guardVarsInitializer && guardClass && guardVar) || (!guardVarsInitializer && !guardClass && !guardVar),
        "triple (guard class, guard var and guard vars initializer) is expected to be initialized or not initialized");

    for (const auto& importedClass : patched->GetImportedClasses()) {
        if (importedClass->GetSrcCodeIdentifier() == "Exception") {
            exceptionDef = importedClass;
            for (const auto& method : importedClass->GetMethods()) {
                if (method->IsConstructor() && method->GetNumOfParams() == 2 &&
                    method->GetParam(1)->GetType() == builder->GetStringTy()) {
                    exceptionInitDef = method;
                    break;
                }
            }
            break;
        }
    }

    error(exceptionDef, "unable to find Exception definition");
    error(exceptionInitDef, "unable to find Exception.<init>(String) definition");
}

void PatchGen::createPatch()
{
    mark();
    markInternalFuncsAsNonImportedAreReachableFromPatchable();
    genPatch();
}

void PatchGen::mark()
{
#if DEBUG
    std::cout << "======================= mark phase =======================" << std::endl;
#endif

    const auto patchedGlobalFuncs = patchedPackage->GetGlobalFunctions();
    if (sourcePackage) {
        error(*sourcePackage == *patchedPackage, "packages differ");

        markContext.markAsPatchable(patchedPackage->GetPackageInitFunc());
        markContext.markAsPatchable(patchedPackage->GetPackageLiteralInitFunc());

        std::optional<std::string> patchClassName;
        if (guardClass) {
            patchClassName = PatchableName(guardClass).genPatchClassName();
        }

        const auto patchedClasses = patchedPackage->GetCurPkgCustomTypeDef();
        std::unordered_set<CustomTypeDef*, CustomDefHasher, CustomDefEquality> patchedTypeDefsSet(
            patchedClasses.begin(), patchedClasses.end());
        for (const auto& sourceTypeDef : sourcePackage->GetCurPkgCustomTypeDef()) {
            const auto customKind = sourceTypeDef->GetCustomKind();

            // TODO support enums and extends later
            if (customKind != TYPE_CLASS && customKind != TYPE_STRUCT) {
#if DEBUG
                std::cout << "Only class or struct type def are supported now, but "
                          << sourceTypeDef->GetIdentifierWithoutPrefix() << " is not one of them" << std::endl;
#endif
                continue;
            }

            std::string typeKind;
            switch (customKind) {
                case TYPE_STRUCT:
                    typeKind = "struct";
                    break;
                case TYPE_CLASS:
                    typeKind = "class";
                    break;
                case TYPE_ENUM:
                    typeKind = "enum";
                    break;
                case TYPE_EXTEND:
                    typeKind = "extend";
                    break;
            }

#if DEBUG
            std::cout << typeKind << " " << sourceTypeDef->GetIdentifierWithoutPrefix() << std::endl;
#endif

            const auto patchedTypeDef = patchedTypeDefsSet.find(sourceTypeDef);
            error(patchedTypeDef != patchedTypeDefsSet.end(),
                typeKind + " " + sourceTypeDef->GetSrcCodeIdentifier() + " was not found in patched version");

            if (!patchClassName.has_value() || patchClassName.value() != sourceTypeDef->GetSrcCodeIdentifier()) {
                markContext.markTypeAsImported(sourceTypeDef->GetIdentifierWithoutPrefix());
            }

            for (const auto& sourceInstanceVar : sourceTypeDef->GetDirectInstanceVars()) {
#if DEBUG
                std::cout << "instance var " + sourceInstanceVar.name << std::endl;
#endif
                // TODO check if var was not removed
            }
        }

        const auto sourceGlobalFuncs = sourcePackage->GetGlobalFunctions();
        std::unordered_set<Function*, FunctionHasher, FunctionEquality> patchedGlobalFuncsSet(
            patchedGlobalFuncs.begin(), patchedGlobalFuncs.end());

        const auto packageName = patchedPackage->GetName();
        // class instance/static methods are also included here
        const auto patchableGuardVarsInitializer = PatchableName(patchedPackage).genGuardVarInitializedName();
        for (const auto sourceFunc : sourceGlobalFuncs) {
            const auto parentTypeDef = sourceFunc->GetParentCustomTypeDef();
#if DEBUG
            std::cout << "source ";
            if (parentTypeDef) {
                std::cout << (sourceFunc->TestAttr(Attribute::STATIC) ? "static " : "");
            } else {
                std::cout << "global ";
            }
            std::cout << "func ";
            std::cout << sourceFunc->GetIdentifierWithoutPrefix() << std::endl;
#endif
            // All these functions are called in package inits, so no need to create diff for them.
            if (const auto funcKind = sourceFunc->GetFuncKind(); packageName == sourceFunc->GetPackageName() &&
                (funcKind == GLOBALVAR_INIT || funcKind == ANNOFACTORY_FUNC ||
                    sourceFunc->GetSrcCodeIdentifier() == patchableGuardVarsInitializer)) {
#if DEBUG
                std::cout << "ignoring as one is global var initializer or annotation factory function "
                             "that belongs to the current package"
                          << std::endl;
#endif
                continue;
            }

            if (parentTypeDef && parentTypeDef->GetSrcCodeIdentifier() == patchClassName) {
                // no need to compare patch stubs during mark phase
                continue;
            }

            // TODO support lambdas and generics
            const auto patchedGlobalFunc = patchedGlobalFuncsSet.find(sourceFunc);
            error(patchedGlobalFunc != patchedGlobalFuncsSet.end(),
                "func " + sourceFunc->GetIdentifierWithoutPrefix() + " was not found in patched version");
            markFunc(sourceFunc, *patchedGlobalFunc);
        }

        markFuncsAsNonPatchable(patchedGlobalFuncs, std::unordered_set<Function*, FunctionHasher, FunctionEquality>(
                sourceGlobalFuncs.begin(), sourceGlobalFuncs.end()));

        const auto patchedGlobalVars = patchedPackage->GetGlobalVars();
        const std::unordered_set<GlobalVar*, GlobalVarHasher, GlobalVarEquality> patchedGlobalVarsSet(
            patchedGlobalVars.begin(), patchedGlobalVars.end());
        // class static vars are also included here
        for (const auto sourceVar : sourcePackage->GetGlobalVars()) {
            const std::string varKind = sourceVar->GetParentCustomTypeDef() ? "static" : "global";
#if DEBUG
            std::cout << varKind << " var ";
            std::cout << sourceVar->GetIdentifierWithoutPrefix() << std::endl;
#endif
            const auto patchedGlobalVar = patchedGlobalVarsSet.find(sourceVar);
            error(patchedGlobalVar != patchedGlobalVarsSet.end(),
                varKind + " var " + sourceVar->GetIdentifierWithoutPrefix() + " was not found in patched version");
            markContext.markVarAsImported(*patchedGlobalVar);
        }

        // TODO provide other checks related to new method/field addition, etc.
    } else {
        markFuncsAsNonPatchable(patchedGlobalFuncs, {});
    }
}

void PatchGen::markFunc(const Function* sourceFunc, Function* patchedFunc)
{
    if (!sourceFunc->IsFuncWithBody()) {
        if (!sourceFunc->IsImportedFunc()) {
            markContext.markFuncAsImported(patchedFunc);
        }
        return;
    }

    const auto isPatchableSourceFunc = isPatchable(sourceFunc);
    error(!(isPatchableSourceFunc ^ isPatchable(patchedFunc)),
        "function " + sourceFunc->GetSrcCodeIdentifier() + " must be patchable both in source and patched versions");
    error(!(sourceFunc->IsFuncWithBody() ^ patchedFunc->IsFuncWithBody()),
        "function " + sourceFunc->GetSrcCodeIdentifier() + " must have body both in source and patched versions");

    if (sourceFunc->GetAttributeInfo().GetRawAttrs() != patchedFunc->GetAttributeInfo().GetRawAttrs()) {
        error("attributes of non-patchable function " + sourceFunc->GetSrcCodeIdentifier() +
            " must stay unchanged in patched version");
    }

    if (patchedFunc->IsLambda()) {
        markContext.markFuncAsDeleted(patchedFunc);
    } else {
        markContext.markFuncAsImported(patchedFunc);
    }

    if (FuncCodeEquality().areEqual(sourceFunc->GetBody(), patchedFunc->GetBody())) {
        return;
    }

    if (isPatchableSourceFunc) {
        markContext.markAsPatchable(patchedFunc);
    } else {
        error(
            "non-patchable function " + sourceFunc->GetSrcCodeIdentifier() + " must stay unchanged in patched version");
    }
}

void PatchGen::markFuncsAsNonPatchable(const std::vector<Function*>& patchedGlobalFuncs,
    const std::unordered_set<Function*, FunctionHasher, FunctionEquality>& sourceGlobalFuncs)
{
    for (const auto patchedFunc : patchedGlobalFuncs) {
#if DEBUG
        std::cout << "patched ";
        if (patchedFunc->GetParentCustomTypeDef()) {
            std::cout << (patchedFunc->TestAttr(Attribute::STATIC) ? "static " : "");
        } else {
            std::cout << "global ";
        }
        std::cout << "func ";
        std::cout << patchedFunc->GetIdentifierWithoutPrefix() << std::endl;
#endif
        if (isPatchable(patchedFunc) && sourceGlobalFuncs.find(patchedFunc) == sourceGlobalFuncs.end()) {
            markContext.markFuncToMakeNonPatchable(patchedFunc);
        }
    }
}

void PatchGen::markInternalFuncsAsNonImportedAreReachableFromPatchable()
{
    std::unordered_set<const Function*> visited;
    for (const auto patchable : markContext.getPatchables()) {
        markInternalFuncAsNonImportedAreReachableFromPatchable(patchable, visited);
    }
}

void PatchGen::markInternalFuncAsNonImportedAreReachableFromPatchable(const Function* func, std::unordered_set<const Function*>& visited)
{
    Visitor::Visit(
        *func, [](Expression&) { return VisitResult::CONTINUE; },
        [&](Expression& e) {
            const auto apply = dynamic_cast<Apply*>(&e);
            if (!apply) {
                return VisitResult::CONTINUE;
            }

            if (const auto callee = dynamic_cast<Function*>(apply->GetCallee());
                callee && callee->Get<LinkTypeInfo>() == Cangjie::Linkage::INTERNAL) {
                if (callee->IsLambda()) {
                    markContext.unmarkFuncAsDeleted(callee);
                } else {
                    markContext.unmarkFuncAsImported(callee);
                }
                const auto [_, added] = visited.insert(callee);
                if (added) {
                    markInternalFuncAsNonImportedAreReachableFromPatchable(callee, visited);
                }
            }

            return VisitResult::CONTINUE;
        });
}

void PatchGen::genPatch()
{
#if DEBUG
    std::cout << "======================= remove unused code phase =======================" << std::endl;
#endif
    makeNewPatchableFuncsAsNonPatchable();

    if (sourcePackage) {
#if DEBUG
        std::cout << "======================= gen patch phase =======================" << std::endl;
#endif
        const auto patches = genPatches();
        importFuncs(patches);
        importVars();
        importTypes();
        removeRedundantFunctions();
        updatePackageInits();
    }

    // TODO if no patches are generated, remove guard classes, getters/setters, guard var initializer (see $GF, $GVI, $GV in newFunc1 test)

    CHIRSerializer::Serialize(*patchedPackage, fileName, ToCHIR::OPT);

#if TEST
    const std::unordered_set<CHIRChecker::Rule> rules{
        CHIRChecker::Rule::EMPTY_BLOCK,
        CHIRChecker::Rule::GET_INSTANTIATE_VALUE_SHOULD_GONE,
        CHIRChecker::Rule::RETURN_TYPE_NEED_BE_VOID,
        CHIRChecker::Rule::CHECK_FUNC_BODY,
        CHIRChecker::Rule::CHIR_GET_RTTI_STATIC_TYPE,
    };
    if (!CHIRChecker(*patchedPackage, {}, *builder).CheckPackage(rules)) {
        error("checker failed");
    }
#endif
}

/**
  The function removes redundant functions from the package:

  -   Removes empty initializers are located in current package.
      These kinds of initializers are removed during LLVM-bitcode codegen, but remain in CHIR.
      We remove them, as don't want to see them in final patch.

      CHIR pseudocode:

      before:

      func packageInit() {
      ...
      Block #x:  // predecessors: [#0]
        %0: Bool = Constant(true)
        %1: Unit = Store(%1, @$has_applied_pkg_init_func)
        %2: Unit = Apply(packageInitForDependencies)
        %3: Unit = Intrinsic(preinitialize, )
        %4: Unit = Apply(gvInit1)
        %5: Unit = Apply(gvInit2)
        ...
      }

      func gvInit1() {
      Block #0:
        Exit()
      }

      func gvInit2() {
      Block #0:
        Exit()
      }

      after:

      func packageInit() {
      ...
      Block #x:  // predecessors: [#0]
        %0: Bool = Constant(true)
        %1: Unit = Store(%1, @$has_applied_pkg_init_func)
        %2: Unit = Apply(packageInitForDependencies)
        %3: Unit = Intrinsic(preinitialize, )
        ...
      }

  -   Removes the functions are marked to be deleted, e.g. lambdas are unreachable from patchable functions
 */
void PatchGen::removeRedundantFunctions() const
{
    const auto packageName = patchedPackage->GetName();
    const auto isEmptyInitializer = [&](const Function* func) {
        if (!func->TestAttr(Attribute::INITIALIZER) || func->TestAttr(Attribute::IMPORTED) || !func->IsFuncWithBody() ||
            func->Get<LinkTypeInfo>() != Cangjie::Linkage::INTERNAL) {
            return false;
        }
        const auto blocks = func->GetBody()->GetBlocks();
        if (blocks.size() != 1) {
            return false;
        }
        const auto entryBlock = blocks.front();
        CJC_ASSERT_WITH_MSG(entryBlock == func->GetEntryBlock(), "expected to find entry block");
        if (!entryBlock->GetNonTerminatorExpressions().empty()) {
            return false;
        }
        return entryBlock->GetTerminator()->GetExprKind() == ExprKind::EXIT;
    };

    const auto removeEmptyInitializersCalls = [&](const Function* packageInit) {
        std::unordered_set<std::string> initializersToRemove;
        Visitor::Visit(
            *packageInit, [](Expression&) { return VisitResult::CONTINUE; },
            [&](Expression& e) {
                if (e.GetExprKind() == ExprKind::APPLY) {
                    const auto apply = static_cast<Apply*>(&e);
                    if (const auto callee = apply->GetCallee(); callee->IsFunc()) {
                        if (const auto func = static_cast<Function*>(callee); isEmptyInitializer(func)) {
                            initializersToRemove.insert(func->GetIdentifierWithoutPrefix());
                            apply->RemoveSelfFromBlock();
                        }
                    }
                }
                return VisitResult::CONTINUE;
            });
        return initializersToRemove;
    };

    auto initializersToRemove = removeEmptyInitializersCalls(patchedPackage->GetPackageInitFunc());
    const auto initializersToRemoveLiteral = removeEmptyInitializersCalls(patchedPackage->GetPackageLiteralInitFunc());
    initializersToRemove.insert(initializersToRemoveLiteral.begin(), initializersToRemoveLiteral.end());

    const auto funcsToDelete = markContext.getFuncsToDelete();
    initializersToRemove.insert(funcsToDelete.begin(), funcsToDelete.end());

    std::vector<Function*> newGlobalFunctions;
    for (const auto& func : patchedPackage->GetGlobalFunctions()) {
        if (initializersToRemove.find(func->GetIdentifierWithoutPrefix()) == initializersToRemove.end()) {
            newGlobalFunctions.emplace_back(func);
        }
    }
    patchedPackage->SetAllGlobalFuncs(std::move(newGlobalFunctions));
}

/*
  Replace access to package inits flags to corresponding accessors' call.
*/
void PatchGen::updatePackageInits() const
{
    GlobalVar* packageInitFlag = nullptr;
    GlobalVar* packageInitLiteralFlag = nullptr;
    for (const auto globalVar : patchedPackage->GetGlobalVars()) {
        const auto name = globalVar->GetSrcCodeIdentifier();
        if (packageInitFlag && packageInitLiteralFlag) {
            break;
        }
        if (name == GV_PKG_INIT_ONCE_FLAG) {
            packageInitFlag = globalVar;
        } else if (name == "has_invoked_pkg_init_literal") {
            packageInitLiteralFlag = globalVar;
        }
    }
    error(packageInitFlag, "unable to find package init flag");
    error(packageInitLiteralFlag, "unable to find package init literal flag");

    const auto packagePatchableName = PatchableName(patchedPackage);

    Function* packageInitFlagGetter = nullptr;
    Function* packageInitFlagSetter = nullptr;
    Function* packageInitLiteralFlagGetter = nullptr;
    Function* packageInitLiteralFlagSetter = nullptr;
    for (const auto globalFunc : patchedPackage->GetGlobalFunctions()) {
        const auto name = globalFunc->GetSrcCodeIdentifier();
        if (packageInitFlagGetter && packageInitFlagSetter && packageInitLiteralFlagGetter && packageInitLiteralFlagSetter) {
            break;
        }
        if (name == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, true)) {
            packageInitFlagGetter = globalFunc;
        } else if (name == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_INIT, false)) {
            packageInitFlagSetter = globalFunc;
        } else if (name == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, true)) {
            packageInitLiteralFlagGetter = globalFunc;
        } else if (name == packagePatchableName.genPackageInitFlagAccessor(PatchableName::PackageInitAccessorKind::PACKAGE_LITERAL_INIT, false)) {
            packageInitLiteralFlagSetter = globalFunc;
        }
    }
    error(packageInitFlagGetter, "unable to find package init flag getter");
    error(packageInitFlagSetter, "unable to find package init flag setter");
    error(packageInitLiteralFlagGetter, "unable to find package init literal flag getter");
    error(packageInitLiteralFlagSetter, "unable to find package init literal flag setter");

    const auto replaceFlagAccessToFlagAccessorCall = [&](const Function* packageInit, const GlobalVar* flag,
                                                         Function* getter, Function* setter) {
        Visitor::Visit(
            *packageInit, [](Expression&) { return VisitResult::CONTINUE; },
            [&](Expression& e) {
                if (const auto kind = e.GetExprKind(); kind == ExprKind::LOAD) {
                    const auto load = static_cast<Load*>(&e);
                    if (const auto callee = load->GetLocation(); callee == flag) {
                        const auto replacement = builder->CreateExpression<Apply>(getter->GetReturnType(), getter,
                            FuncCallContext{
                                .args = {},
                                .instTypeArgs = {},
                                .thisType = nullptr,
                            },
                            e.GetParentBlock());
                        e.ReplaceWith(*replacement);
                    }
                } else if (kind == ExprKind::STORE) {
                    const auto store = static_cast<Store*>(&e);
                    if (const auto callee = store->GetLocation(); callee == flag) {
                        const auto replacement = builder->CreateExpression<Apply>(setter->GetReturnType(), setter,
                            FuncCallContext{
                                .args = {store->GetValue()},
                                .instTypeArgs = {},
                                .thisType = nullptr,
                            },
                            e.GetParentBlock());
                        e.ReplaceWith(*replacement);
                    }
                }

                return VisitResult::CONTINUE;
            });
    };

    replaceFlagAccessToFlagAccessorCall(patchedPackage->GetPackageInitFunc(), packageInitFlag, packageInitFlagGetter, packageInitFlagSetter);
    replaceFlagAccessToFlagAccessorCall(patchedPackage->GetPackageLiteralInitFunc(), packageInitLiteralFlag, packageInitLiteralFlagGetter,
        packageInitLiteralFlagSetter);
}

void PatchGen::removeGuardChecks(const Function* patchable) const
{
#if DEBUG
    std::cout << "patchable before guard checks removal: " << std::endl;
    std::cout << patchable->ToString(0) << std::endl;
#endif

    const auto patchableBody = patchable->GetEntryBlock();

    const auto patchableBlockTerminator = patchableBody->GetTerminator();
    error(patchableBlockTerminator->GetExprKind() == ExprKind::BRANCH, "guard var flag check was not found");
    const auto entryBlockTerminator = static_cast<Branch*>(patchableBlockTerminator);
    error(entryBlockTerminator, "expected to have branch terminator in entry block");

    const auto falseBlock = entryBlockTerminator->GetFalseBlock();
    const auto falseBlockTerminator = falseBlock->GetTerminator();
    error(falseBlockTerminator->GetExprKind() == ExprKind::BRANCH, "guard check was not found");
    const auto falseBlockBranchTerminator = static_cast<Branch*>(falseBlockTerminator);

    const auto trueBlock = falseBlockBranchTerminator->GetTrueBlock();
    if (isPackageInit(patchable, patchedPackage)) {
        patchable->GetBody()->SetEntryBlock(entryBlockTerminator->GetTrueBlock());
        patchableBody->RemoveSelfFromBlockGroup();

        const auto t = trueBlock->GetTerminator();
        error(t->GetExprKind() == ExprKind::GOTO, "guard check was not found");
        static_cast<GoTo*>(t)->GetDestination()->RemoveSelfFromBlockGroup();
        trueBlock->RemoveSelfFromBlockGroup();

    } else {
        trueBlock->RemoveSelfFromBlockGroup();

        const PatchableName patchableName(patchable);

        const GlobalVar* guardVarFlag = nullptr;
        const auto guardVarFlagName = patchableName.genGuardVarFlagName(false);
        // TODO optimize collect all guard var flags once.
        for (const auto& globalVar : patchedPackage->GetGlobalVars()) {
            if (globalVar->GetSrcCodeIdentifier() == guardVarFlagName) {
                guardVarFlag = globalVar;
                break;
            }
        }
        error(guardVarFlag, "guard var flag is expected to be initialized");

        const auto patchableExpressions = patchableBody->GetNonTerminatorExpressions();
        const auto loadGuardVarCandidate = patchableExpressions[patchableExpressions.size() - 2];
        const auto loadGuardVar = dynamic_cast<Load*>(loadGuardVarCandidate);
        if (loadGuardVar && loadGuardVar->GetLocation() == guardVarFlag) {
            loadGuardVar->RemoveSelfFromBlock();
        } else {
            error("expected to delete Load(guardVar) expr, but '" + loadGuardVarCandidate->ToString(0) + "' was found");
        }
        const auto notExprCandidate = patchableExpressions.back();
        if (const auto notExpr = dynamic_cast<UnaryExpression*>(notExprCandidate);
            notExpr && notExpr->GetExprKind() == ExprKind::NOT && notExpr->GetOperand() == loadGuardVar->GetResult()) {
            notExpr->RemoveSelfFromBlock();
        } else {
            error("expected to delete NOT(Load(guardVar)) expr, but '" + notExprCandidate->ToString(0) + "' was found");
        }

        const auto usersCodeBlock = entryBlockTerminator->GetTrueBlock();
        for (const auto expr : usersCodeBlock->GetNonTerminatorExpressions()) {
            expr->MoveBefore(entryBlockTerminator);
        }
        entryBlockTerminator->RemoveSelfFromBlock();
        usersCodeBlock->GetTerminator()->MoveTo(*patchableBody);
        usersCodeBlock->RemoveSelfFromBlockGroup();
    }

    falseBlockBranchTerminator->GetFalseBlock()->RemoveSelfFromBlockGroup();
    falseBlock->RemoveSelfFromBlockGroup();

#if DEBUG
    std::cout << "patchable after guard checks removal: " << std::endl;
    std::cout << patchable->ToString(0) << std::endl;
#endif
}

/**
 * - Removes guard checks for the function
 * - Remove corresponding guard var
 * - Remove corresponding methods in guard/patch classes
 * - Mark corresponding global functions as unreachable
 * // TODO remove @patchable AnnoInfo, if possible
 */
void PatchGen::makeNewPatchableFuncsAsNonPatchable()
{
    if (!guardClass) {
#if DEBUG
        std::cout << "no guard class was found" << std::endl;
        error(markContext.getFuncsToMakeNonPatchable().empty(), "patchable functions are found, but guard class wasn't initialized");
#endif
        return;
    }

    patchClass = findPatchClass(patchedPackage, guardClass);
    error(patchClass, "patch class is expected to be initialized");
#if DEBUG
    std::cout << "patch class:" << std::endl;
    std::cout << patchClass->ToString() << std::endl;
#endif

    for (const auto func : markContext.getFuncsToMakeNonPatchable()) {
        removeGuardChecks(func);
        const auto patchableName = PatchableName(func);

        const auto guardVarFlagName = patchableName.genGuardVarFlagName(false);
        std::vector<GlobalVar*> newGlobalVars;
        for (const auto globalVar : patchedPackage->GetGlobalVars()) {
            if (globalVar->GetSrcCodeIdentifier() != guardVarFlagName) {
                newGlobalVars.emplace_back(globalVar);
            }
        }
        patchedPackage->SetAllGlobalVars(std::move(newGlobalVars));

        const auto guardMethodName = patchableName.genGuardMethodName(PatchableName::GuardMethodKind::PLAIN);

        std::vector<Function*> newGuardClassMethod;
        for (const auto method : guardClass->GetMethods()) {
            if (method->GetSrcCodeIdentifier() != guardMethodName) {
                newGuardClassMethod.emplace_back(method);
            }
        }
        guardClass->SetMethods(newGuardClassMethod);

        std::vector<Function*> newPatchClassMethods;
        for (const auto method : patchClass->GetMethods()) {
            if (method->GetSrcCodeIdentifier() != guardMethodName) {
                newPatchClassMethods.emplace_back(method);
            }
        }
        patchClass->SetMethods(newPatchClassMethods);

        // Here we can't remove redundant global functions, as their references exist in vtable.
        // We don't want to use vtable CHIR C++ API as there is no stdx libchir analogue.
        // So marking these functions as unreachable lets cbc-compiler to recognize and remove them.
        std::vector<Function*> globalFuncs;
        for (const auto globalFunc : patchedPackage->GetGlobalFunctions()) {
            if (globalFunc->GetSrcCodeIdentifier() == guardMethodName) {
                globalFunc->EnableAttr(Attribute::UNREACHABLE);
            }
        }
    }
}

/*
  For CJ source code:

  func foo() {
    println("Hello")
  }

  and guard checks generated by cjc plugin:

  func foo() : Unit  {
  Block #0:
    [ret] %0: Unit& = Allocate(Unit)
    %5: Bool = Load(@$fooPatchVarFlag)
    %6: Bool = Not(%5)
    Branch(%6, #1, #3)
  Block #1:
    %1: Struct-_CNat6StringE = Constant("hello")
    %2: Unit = Apply(@_CNat7printlnHRNat6StringE, %1)
    %3: Unit = Constant(unit)
    %4: Unit = Store(%3, %0)
    Exit()
  Block #3:
    %7: Enum-_CNat6OptionIG_E<Class-$Patch&> = Load(@$patchVar)
    %8: Bool = Field(%7, 0)
    %9: Bool = Constant(false)
    %10: Bool = Equal(%8, %9)
    Branch(%10, #4, #5)
  Block #4:
    %14: Tuple(Bool,Class-$Patch&) = TypeCast(%7)
    [readOnly] %15: Class-$Patch& = Field(%14, 1)
    %16: Unit = Invoke(ThisType: Class-$Patch&, $fooPatch: (Class-$Patch&) -> Unit, %15)
    %17: Unit = Constant(unit)
    %18: Unit = Store(%17, %0)
    Exit()
  Block #5:
    %11: Struct-_CNat6StringE = Constant("should not reach here")
    %12: Class-_CNat9ExceptionE& = Allocate(Class-_CNat9ExceptionE)
    %13: Unit = Apply(ThisType: Class-_CNat9ExceptionE&, @_CNat9Exception6<init>HRNat6StringE, %12, %11)
    RaiseException(%12)

  the function

  - removes the guard checks and generate the patch method in already generated patch class:

      CHIR pseudocode:
      class @$PatchImpl <: @$Patch {
        [protected] [override] [final] func $fooPatch(%0: Class-$PatchImpl&) -> Unit {
          %1: Struct-_CNat6StringE = Constant("hello")
          %2: Unit = Apply(@_CNat7printlnHRNat6StringE, %1)
          %3: Unit = Constant(unit)
          %4: Unit = Store(%3, %1)
          Exit()
        }
      }

  - initialize patch class and enable corresponding guard var and flag

*/
std::unordered_set<Function*> PatchGen::genPatches()
{
    std::unordered_set<Function*> patches;
    for (const auto patchable : markContext.getPatchables()) {
#if DEBUG
        std::cout << "patchable before transformation: " << std::endl;
        std::cout << patchable->ToString(0) << std::endl;
#endif

        removeGuardChecks(patchable);

        if (isPackageInit(patchable, patchedPackage)) {
            continue;
        }

        if (!patchClass) {
            error(guardClass, "guard class is expected be initialized");
            patchClass = findPatchClass(patchedPackage, guardClass);
            error(patchClass, "patch class is expected to be initialized");
        }
#if DEBUG
        std::cout << "patch class:" << std::endl;
        std::cout << patchClass->ToString() << std::endl;
#endif

        const PatchableName patchableName(patchable);

        Function* patchMethod = nullptr;
        const auto patchMethodName = patchableName.genGuardMethodName(PatchableName::GuardMethodKind::PLAIN);
        // TODO collect all patch methods once?
        for (const auto& method : patchClass->GetMethods()) {
            if (method->GetSrcCodeIdentifier() == patchMethodName) {
                patchMethod = method;
                break;
            }
        }
        error(patchMethod, "patch method is expected to be initialized");

#if DEBUG
        std::cout << "patch method before: " << std::endl;
        std::cout << patchMethod->ToString(0) << std::endl;
#endif

        const auto patchableBody = patchable->GetEntryBlock();

        const auto currRetVal = patchable->GetReturnValue();
        const bool needRetVal = patchable->HasReturnValue();
        int retValIdx = -1;
        int exprIdx = -1;
        for (const auto expr : patchableBody->GetNonTerminatorExpressions()) {
            exprIdx++;
            if (needRetVal) {
                if (expr->GetResult() == currRetVal) {
                    retValIdx = exprIdx;
                    break;
                }
            }
        }

        patchMethod->ReplaceBody(*patchable->GetBody());

        const auto patchEntryBlock = patchMethod->GetEntryBlock();
        const auto patchableKind = patchable->GetFuncKind();
        const auto isStructCtor = patchableKind == STRUCT_CONSTRUCTOR || patchableKind == PRIMAL_STRUCT_CONSTRUCTOR;

        const auto patchedFuncParams = GetFuncParams(*patchEntryBlock->GetParentBlockGroup());
        // We need to regenerate all the values' identifiers, otherwise it might contain some local var duplicates after patch generation on the previous steps.
        // The only known approach to do that is to clone existing expressions.
        std::unordered_map<Value*, Value*> clonedTerminators;
        Visitor::Visit(
            *patchMethod, [](Expression&) { return VisitResult::CONTINUE; },
            [&](Expression& e) {
                Expression* clonedExpr = &e;
                if (e.IsTerminator()) {
                    // We have to handle all expression-like terminators as they might be used as operands in other
                    // expressions.
                    if (const auto result = e.GetResult()) {
                        clonedExpr = e.Clone(*builder, *e.GetParentBlock());
                        clonedExpr->CopyBaseInfoFrom(e);
                        e.RemoveSelfFromBlock();
                        clonedTerminators.emplace(result, clonedExpr->GetResult());
                    }
                } else {
                    clonedExpr = e.Clone(*builder, *e.GetParentBlock());
                    clonedExpr->CopyBaseInfoFrom(e);
                    e.ReplaceWith(*clonedExpr);
                }
                for (const auto operand : clonedExpr->GetOperands()) {
                    if (operand->IsParameter()) {
                        // We have to update param indices as patch function has its own receiver.
                        for (int i = 0; i < patchedFuncParams.size(); i++) {
                            if (const auto param = patchedFuncParams[i];
                                param->GetIdentifier() == operand->GetIdentifier()) {
                                const auto newOperand = isStructCtor ? param : patchedFuncParams[i + 1];
                                clonedExpr->ReplaceOperand(operand, newOperand);
                                break;
                            }
                        }
                    } else if (const auto clonedTerminator = clonedTerminators.find(operand);
                        clonedTerminator != clonedTerminators.end()) {
                        clonedExpr->ReplaceOperand(operand, clonedTerminator->second);
                    }
                }
                return VisitResult::CONTINUE;
            });

        if (isStructCtor) {
            const auto returnType = patchMethod->GetReturnType();
            CJC_ASSERT_WITH_MSG(
                returnType->IsStruct(), "expected the patch for struct constructor to have a struct type");

            const auto allocStruct =
                builder->CreateExpression<Allocate>(builder->GetType<RefType>(returnType), returnType, patchEntryBlock);
            allocStruct->MoveBefore(patchEntryBlock->GetExpressions().front());
            const auto allocatedRetVal = allocStruct->GetResult();
            patchMethod->SetReturnValue(*allocatedRetVal);

            const auto thisParam = patchedFuncParams.front();
            // update parameter indices in expression operands, as patch is generated in separate class with its own
            // receiver.
            for (const auto expr : GetNonDebugUsers(*thisParam)) {
                expr->ReplaceOperand(thisParam, allocatedRetVal);
            }
        } else {
            if (retValIdx >= 0) {
                const auto newRetVal = patchEntryBlock->GetExpressions().at(retValIdx)->GetResult();
                patchMethod->SetReturnValue(*newRetVal);
            } else if (patchableKind == CLASS_CONSTRUCTOR || patchableKind == PRIMAL_STRUCT_CONSTRUCTOR) {
                const auto allocUnit = builder->CreateExpression<Allocate>(
                    builder->GetType<RefType>(builder->GetUnitTy()), builder->GetUnitTy(), patchEntryBlock);
                allocUnit->MoveBefore(patchEntryBlock->GetTerminator());
                patchMethod->SetReturnValue(*allocUnit->GetResult());
            }
        }

        enableGuardVarFlag(patchableName);

#if DEBUG
        std::cout << "patch method after: " << std::endl;
        std::cout << patchMethod->ToString(0) << std::endl;
#endif

        patches.insert(patchMethod);
    }
    if (!patches.empty()) {
        instantiatePatchClass();
    }
    return patches;
}

/*
  CHIR pseudocode:

  func $guardVarsInit() {
    ..
    %0: Class-$PatchImpl& = Allocate(Class-$PatchImpl)
    %1: Class-$Patch& = TypeCast(%0)
    %2: Bool = Constant(false)
    %3: Enum-_CNat6OptionIG_E<Class-$Patch&> = Tuple(%2, %1)
    %4: Unit = Store(%3, @$patchVar)
  }
 */
void PatchGen::instantiatePatchClass() const
{
    const auto guardVarsInitFuncBody = guardVarsInitializer->GetEntryBlock();
    const auto guardVarsInitFuncTerminator = guardVarsInitFuncBody->GetTerminator();

    const auto patchClassType = patchClass->GetType();
    const auto alloc = builder->CreateExpression<Allocate>(
        builder->GetType<RefType>(patchClassType), patchClassType, guardVarsInitFuncBody);
    alloc->MoveBefore(guardVarsInitFuncTerminator);

    const auto typeCast = builder->CreateExpression<ClassStaticCast>(
        builder->GetType<RefType>(guardClass->GetType()), alloc->GetResult(), guardVarsInitFuncBody);
    typeCast->MoveAfter(alloc);

    const auto falseExpr =
        builder->CreateConstantExpression<BoolLiteral>(builder->GetBoolTy(), guardVarsInitFuncBody, false);
    falseExpr->MoveAfter(typeCast);

    const auto guardVarType = dynamic_cast<RefType*>(guardVar->GetType())->GetBaseType();
    const auto tuple = builder->CreateExpression<Tuple>(
        guardVarType, std::vector<Value*>{falseExpr->GetResult(), typeCast->GetResult()}, guardVarsInitFuncBody);
    tuple->MoveAfter(falseExpr);

    const auto storeGuardVar =
        builder->CreateExpression<Store>(builder->GetUnitTy(), tuple->GetResult(), guardVar, guardVarsInitFuncBody);
    storeGuardVar->MoveAfter(tuple);
}

/*
  CHIR pseudocode:

  @$fooPatchVarFlag: Bool& = false

  func $guardVarsInit() {
    ..
    %0: Bool = Constant(true)
    %1: Unit = Store(%0, @$fooPatchVarFlag)
    ..
  }
 */
void PatchGen::enableGuardVarFlag(const PatchableName& patchableName) const
{
    GlobalVar* guardVarFlag = nullptr;
    const auto guardVarFlagName = patchableName.genGuardVarFlagName(false);
    // TODO collect all guard var flags once.
    for (const auto& globalVar : patchedPackage->GetGlobalVars()) {
        if (globalVar->GetSrcCodeIdentifier() == guardVarFlagName) {
            guardVarFlag = globalVar;
            break;
        }
    }
    error(guardVarFlag, "guard var flag is expected to be initialized");

    const auto guardVarsInitFuncBody = guardVarsInitializer->GetEntryBlock();
    const auto guardVarsInitFuncTerminator = guardVarsInitFuncBody->GetTerminator();

    const auto trueExpr =
        builder->CreateConstantExpression<BoolLiteral>(builder->GetBoolTy(), guardVarsInitFuncBody, true);
    trueExpr->MoveBefore(guardVarsInitFuncTerminator);

    const auto storeToGuardVarFlag = builder->CreateExpression<Store>(
        builder->GetUnitTy(), trueExpr->GetResult(), guardVarFlag, guardVarsInitFuncBody);
    storeToGuardVarFlag->MoveAfter(trueExpr);
}

void PatchGen::importFuncs(const std::unordered_set<Function*>& patches) const
{
    for (const auto func : markContext.getFuncsToImport()) {
        if (patches.find(func) == patches.end()) {
            func->DestroyFuncBody();
            func->EnableAttr(Attribute::IMPORTED);
        }
    }
}

void PatchGen::importVars() const
{
    for (const auto varToImport : markContext.getVarsToImport()) {
        varToImport->AppendAttributeInfo(varToImport->GetAttributeInfo());
        varToImport->EnableAttr(Attribute::IMPORTED);
    }
}

void PatchGen::importTypes() const
{
    const auto typesToImport = markContext.getTypesToImport();
    std::vector<ClassDef*> newClassDefs;
    std::vector<StructDef*> newStructDefs;
    for (const auto typeDef : patchedPackage->GetCurPkgCustomTypeDef()) {
        const auto kind = typeDef->GetCustomKind();
        if (typesToImport.find(typeDef->GetIdentifierWithoutPrefix()) != typesToImport.end()) {
            typeDef->EnableAttr(Attribute::IMPORTED);
            switch (kind) {
                case TYPE_CLASS: {
                    const auto classDef = static_cast<ClassDef*>(typeDef);
                    patchedPackage->AddImportedClass(classDef);
                    break;
                }
                case TYPE_STRUCT:
                    patchedPackage->AddImportedStruct(static_cast<StructDef*>(typeDef));
                    break;
                case TYPE_ENUM:
                    // TODO support
                    break;
                case TYPE_EXTEND:
                    // TODO support
                    break;
            }

        } else {
            switch (kind) {
                case TYPE_CLASS:
                    newClassDefs.push_back(static_cast<ClassDef*>(typeDef));
                    break;
                case TYPE_STRUCT:
                    newStructDefs.push_back(static_cast<StructDef*>(typeDef));
                    break;
                case TYPE_ENUM:
                    // TODO support
                    break;
                case TYPE_EXTEND:
                    // TODO support
                    break;
            }
        }
    }
    patchedPackage->SetClasses(std::move(newClassDefs));
    patchedPackage->SetStructs(std::move(newStructDefs));
    // TODO support enums, extends
}

Function* PatchGen::findGuardVarsInitializer(const Package* package)
{
    const auto gvitName = PatchableName(package).genGuardVarInitializedName();
    for (const auto func : package->GetGlobalFuncsWithBody()) {
        if (func->GetSrcCodeIdentifier() == gvitName) {
            return func;
        }
    }
    return nullptr;
}

ClassDef* PatchGen::findGuardClass(const Package* package)
{
    const auto gcName = PatchableName(package).genGuardClassName(false);
    for (const auto& classDef : package->GetClasses()) {
        if (classDef->GetSrcCodeIdentifier() == gcName) {
            return classDef;
        }
    }
    return nullptr;
}

ClassDef* PatchGen::findPatchClass(const Package* package, const ClassDef* patchClass)
{
    const auto pcName = PatchableName(patchClass).genPatchClassName();
    for (const auto& classDef : package->GetClasses()) {
        if (classDef->GetSrcCodeIdentifier() == pcName) {
            return classDef;
        }
    }
    return nullptr;
}

GlobalVar* PatchGen::findGuardVar(const Package* package)
{
    const auto gvName = PatchableName(package).genGuardVarName(false);
    for (const auto& globalVar : package->GetGlobalVars()) {
        if (globalVar->GetSrcCodeIdentifier() == gvName) {
            return globalVar;
        }
    }
    return nullptr;
}

/*
  Returns true, if specific check exists in the entry block of the function, false otherwise.

  CHIR pseudocode of the check:

  @$fooPatchVarFlag: Bool&

  func foo() {
    Block #0:
      %0: Bool = Load(@$fooPatchVarFlag)
      %1: Bool = Not(%0)
      Branch(%1, #x, #y)
    ...
  }
 */
bool PatchGen::isPatchable(const Function* func)
{
    if (!func->IsFuncWithBody()) {
        return false;
    }
    const auto entryBlock = func->GetBody()->GetEntryBlock();
    const auto terminator = entryBlock->GetTerminator();
    if (terminator->GetExprKind() != ExprKind::BRANCH) {
        return false;
    }
    const auto condition = static_cast<Branch*>(terminator)->GetCondition();
    if (typeid(*condition) != typeid(LocalVar)) {
        return false;
    }
    auto expr = static_cast<LocalVar*>(condition)->GetExpr();
    if (expr->GetExprKind() != ExprKind::NOT) {
        return false;
    }
    const auto operand = static_cast<UnaryExpression*>(expr)->GetOperand();
    if (typeid(*operand) != typeid(LocalVar)) {
        return false;
    }
    expr = static_cast<LocalVar*>(operand)->GetExpr();
    if (expr->GetExprKind() != ExprKind::LOAD) {
        return false;
    }
    const auto location = static_cast<Load*>(expr)->GetLocation();
    if (typeid(*location) != typeid(GlobalVar)) {
        return false;
    }
    const auto globalVarName = static_cast<GlobalVar*>(location)->GetSrcCodeIdentifier();
    const auto guardVarFlagName = PatchableName(func).genGuardVarFlagName(false);
    return globalVarName == guardVarFlagName;
}

bool PatchGen::isPackageInit(const Function* func, const Package* package)
{
    return func == package->GetPackageInitFunc() || func == package->GetPackageLiteralInitFunc();
}
} // namespace PatchGenerator