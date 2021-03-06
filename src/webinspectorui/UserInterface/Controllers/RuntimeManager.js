/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.RuntimeManager = class RuntimeManager extends WebInspector.Object
{
    constructor()
    {
        super();

        // Enable the RuntimeAgent to receive notification of execution contexts.
        RuntimeAgent.enable();

        this._activeExecutionContext = WebInspector.mainTarget.executionContext;

        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ExecutionContextsCleared, this._frameExecutionContextsCleared, this);
    }

    // Public

    get activeExecutionContext()
    {
        return this._activeExecutionContext;
    }

    set activeExecutionContext(executionContext)
    {
        if (this._activeExecutionContext === executionContext)
            return;

        this._activeExecutionContext = executionContext;

        this.dispatchEventToListeners(WebInspector.RuntimeManager.Event.ActiveExecutionContextChanged);
    }

    evaluateInInspectedWindow(expression, options, callback)
    {
        let {objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, saveResult, sourceURLAppender} = options;

        includeCommandLineAPI = includeCommandLineAPI || false;
        doNotPauseOnExceptionsAndMuteConsole = doNotPauseOnExceptionsAndMuteConsole || false;
        returnByValue = returnByValue || false;
        generatePreview = generatePreview || false;
        saveResult = saveResult || false;
        sourceURLAppender = sourceURLAppender || appendWebInspectorSourceURL;

        console.assert(objectGroup, "RuntimeManager.evaluateInInspectedWindow should always be called with an objectGroup");
        console.assert(typeof sourceURLAppender === "function");

        if (!expression) {
            // There is no expression, so the completion should happen against global properties.
            expression = "this";
        } else if (/^\s*\{/.test(expression) && /\}\s*$/.test(expression)) {
            // Transform {a:1} to ({a:1}) so it is treated like an object literal instead of a block with a label.
            expression = "(" + expression + ")";
        } else if (/\bawait\b/.test(expression)) {
            // Transform `await <expr>` into an async function assignment.
            expression = this._tryApplyAwaitConvenience(expression);
        }

        expression = sourceURLAppender(expression);

        let target = this._activeExecutionContext.target;
        let executionContextId = this._activeExecutionContext.id;

        if (WebInspector.debuggerManager.activeCallFrame) {
            target = WebInspector.debuggerManager.activeCallFrame.target;
            executionContextId = target.executionContext.id;
        }

        function evalCallback(error, result, wasThrown, savedResultIndex)
        {
            this.dispatchEventToListeners(WebInspector.RuntimeManager.Event.DidEvaluate, {objectGroup});

            if (error) {
                console.error(error);
                callback(null, false);
                return;
            }

            if (returnByValue)
                callback(null, wasThrown, wasThrown ? null : result, savedResultIndex);
            else
                callback(WebInspector.RemoteObject.fromPayload(result, target), wasThrown, savedResultIndex);
        }

        if (WebInspector.debuggerManager.activeCallFrame) {
            // COMPATIBILITY (iOS 8): "saveResult" did not exist.
            target.DebuggerAgent.evaluateOnCallFrame.invoke({callFrameId: WebInspector.debuggerManager.activeCallFrame.id, expression, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, saveResult}, evalCallback.bind(this), target.DebuggerAgent);
            return;
        }

        // COMPATIBILITY (iOS 8): "saveResult" did not exist.
        target.RuntimeAgent.evaluate.invoke({expression, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, contextId: executionContextId, returnByValue, generatePreview, saveResult}, evalCallback.bind(this), target.RuntimeAgent);
    }

    saveResult(remoteObject, callback)
    {
        console.assert(remoteObject instanceof WebInspector.RemoteObject);

        // COMPATIBILITY (iOS 8): Runtime.saveResult did not exist.
        if (!RuntimeAgent.saveResult) {
            callback(undefined);
            return;
        }

        function mycallback(error, savedResultIndex)
        {
            callback(savedResultIndex);
        }

        let target = this._activeExecutionContext.target;
        let executionContextId = this._activeExecutionContext.id;

        if (remoteObject.objectId)
            target.RuntimeAgent.saveResult(remoteObject.asCallArgument(), mycallback);
        else
            target.RuntimeAgent.saveResult(remoteObject.asCallArgument(), executionContextId, mycallback);
    }

    getPropertiesForRemoteObject(objectId, callback)
    {
        this._activeExecutionContext.target.RuntimeAgent.getProperties(objectId, function(error, result) {
            if (error) {
                callback(error);
                return;
            }

            let properties = new Map;
            for (let property of result)
                properties.set(property.name, property);

            callback(null, properties);
        });
    }

    // Private

    _frameExecutionContextsCleared(event)
    {
        let contexts = event.data.contexts || [];

        let currentContextWasDestroyed = contexts.some((context) => context.id === this._activeExecutionContext.id);
        if (currentContextWasDestroyed)
            this.activeExecutionContext = WebInspector.mainTarget.executionContext;
    }

    _tryApplyAwaitConvenience(originalExpression)
    {
        let esprimaSyntaxTree;

        // Do not transform if the original code parses just fine.
        try {
            esprima.parse(originalExpression);
            return originalExpression;
        } catch (error) { }

        // Do not transform if the async function version does not parse.
        try {
            esprimaSyntaxTree = esprima.parse("(async function(){" + originalExpression + "})");
        } catch (error) {
            return originalExpression;
        }

        // Assert expected AST produced by our wrapping code.
        console.assert(esprimaSyntaxTree.type === "Program");
        console.assert(esprimaSyntaxTree.body.length === 1);
        console.assert(esprimaSyntaxTree.body[0].type === "ExpressionStatement");
        console.assert(esprimaSyntaxTree.body[0].expression.type === "FunctionExpression");
        console.assert(esprimaSyntaxTree.body[0].expression.async);
        console.assert(esprimaSyntaxTree.body[0].expression.body.type === "BlockStatement");

        // Do not transform if there is more than one statement.
        let asyncFunctionBlock = esprimaSyntaxTree.body[0].expression.body;
        if (asyncFunctionBlock.body.length !== 1)
            return originalExpression;

        // Extract the variable name for transformation.
        let variableName;
        let anonymous = false;
        let declarationKind = "var";
        let awaitPortion;
        let statement = asyncFunctionBlock.body[0];
        if (statement.type === "ExpressionStatement"
            && statement.expression.type === "AwaitExpression") {
            // await <expr>
            anonymous = true;
        } else if (statement.type === "ExpressionStatement"
            && statement.expression.type === "AssignmentExpression"
            && statement.expression.right.type === "AwaitExpression"
            && statement.expression.left.type === "Identifier") {
            // x = await <expr>
            variableName = statement.expression.left.name;
            awaitPortion = originalExpression.substring(originalExpression.indexOf("await"));
        } else if (statement.type === "VariableDeclaration"
            && statement.declarations.length === 1
            && statement.declarations[0].init.type === "AwaitExpression"
            && statement.declarations[0].id.type === "Identifier") {
            // var x = await <expr>
            variableName = statement.declarations[0].id.name;
            declarationKind = statement.kind;
            awaitPortion = originalExpression.substring(originalExpression.indexOf("await"));
        } else {
            // Do not transform if this was not one of the simple supported syntaxes.
            return originalExpression;
        }

        if (anonymous) {
            return `
(async function() {
    try {
        let result = ${originalExpression};
        console.info("%o", result);
    } catch (e) {
        console.error(e);
    }
})();
undefined`;
        }

        return `${declarationKind} ${variableName};
(async function() {
    try {
        ${variableName} = ${awaitPortion};
        console.info("%o", ${variableName});
    } catch (e) {
        console.error(e);
    }
})();
undefined;`;
    }
};

WebInspector.RuntimeManager.ConsoleObjectGroup = "console";
WebInspector.RuntimeManager.TopLevelExecutionContextIdentifier = undefined;

WebInspector.RuntimeManager.Event = {
    DidEvaluate: Symbol("runtime-manager-did-evaluate"),
    DefaultExecutionContextChanged: Symbol("runtime-manager-default-execution-context-changed"),
    ActiveExecutionContextChanged: Symbol("runtime-manager-active-execution-context-changed"),
};
