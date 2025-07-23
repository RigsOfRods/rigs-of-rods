/*
    This code is adopted from https://github.com/zethon/bbcpp
    at commit 852a02dda37a17f458dd68ecf5461141f93edce2.
    See reprint of original license (MIT) in README.txt
*/

#include <cstring>
#include <cctype>
#include "BBDocument.h"

namespace bbcpp
{

BBNode::BBNode(NodeType nodeType, const std::string& name)
    : _name(name), _nodeType(nodeType)
{
    // nothing to do
}

BBText &BBDocument::newText(const std::string &text)
{
    // first try to append this text to the item on top of the stack
    // if that is a BBText object, if not, then see if the last element
    // pushed to BBDocument is a text item, and if so append this to that
    // text
    if (_stack.size() > 0 && _stack.top()->getChildren().size() > 0)
    {
        auto totalChildCnt = _stack.top()->getChildren().size();
        auto textnode = _stack.top()->getChildren().at(totalChildCnt - 1)->downCast<BBTextPtr>(false);
        if (textnode)
        {
            textnode->append(text);
            return *textnode;
        }
    }
    else if (_children.size() > 0)
    {
        auto textnode = _children.back()->downCast<BBTextPtr>(false);
        if (textnode)
        {
            textnode->append(text);
            return *textnode;
        }
    }

    // ok, there was no previous text element so we wil either add this text
    // element as a child of the top item OR we'll add it to the BBDocucment
    // object
    auto textNode = std::make_shared<BBText>(text);
    if (_stack.size() > 0)
    {
        _stack.top()->appendChild(textNode);
    }
    else
    {
        // add this node to the document-node if needed
        appendChild(textNode);
    }

    return *textNode;
}

BBElement& BBDocument::newElement(const std::string &name)
{
    auto newNode = std::make_shared<BBElement>(name);
    if (_stack.size() > 0)
    {
        _stack.top()->appendChild(newNode);  
    }
    else
    {
        // add this node to the document-node if needed
        appendChild(newNode);
    }
 
    _stack.push(newNode);
    return *newNode;
}

BBElement& BBDocument::newClosingElement(const std::string& name)
{
    auto newNode = std::make_shared<BBElement>(name, BBElement::CLOSING);
    if (_stack.size() > 0)
    {
        _stack.top()->appendChild(newNode);  
        _stack.pop();
    }
    else
    {
        appendChild(newNode);
    }

    return *newNode;
}

BBElement& BBDocument::newKeyValueElement(const std::string& name, const ParameterMap& pairs)
{
    auto newNode = std::make_shared<BBElement>(name, BBElement::PARAMETER);
    if (_stack.size() > 0)
    {
        _stack.top()->appendChild(newNode);
    }
    else
    {
        // add this node to the document-node if needed
        appendChild(newNode);
    }

    for (const auto& kv : pairs)
    {
        newNode->setOrAddParameter(kv.first, kv.second);
    }

    _stack.push(newNode);
    return *newNode;
}
  


} // namespace
