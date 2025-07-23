/*
    This code is adopted from https://github.com/zethon/bbcpp
    at commit 852a02dda37a17f458dd68ecf5461141f93edce2.
    See reprint of original license (MIT) in README.txt

    Changes done in this file:
    * Added commentary to the private default constructor of BBDocument
    * Renamed `parseValue` to `parseAttributeValue` for clarity
    *  - added support for quoted attributes
    * `parseAttributeValue()` removed the `IsAlNum()` and all color/url special character checks because:
       > '_' was missing from the original code
       > UTF-8 characters would cause havoc
    * `parseElementName()` - also support singular `[*]` as list item
*/

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <map>
#include <iterator>
#include <cctype>
#include <cstring>

namespace bbcpp
{

inline bool IsDigit(char c)
{
    return ('0' <= c && c <= '9');
}

inline bool IsAlpha(char c)
{
    static const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return (std::strchr(alpha, c) != nullptr);
}

inline bool IsAlNum(char c)
{
    return IsAlpha(c) || IsDigit(c);
}

inline bool IsSpace(char c)
{
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

class BBNode;
class BBText;
class BBElement;
class BBDocument;

using BBNodePtr = std::shared_ptr<BBNode>;
using BBTextPtr = std::shared_ptr<BBText>;
using BBElementPtr = std::shared_ptr<BBElement>;

using BBNodeWeakPtr = std::weak_ptr<BBNode>;
using BBNodeList = std::vector<BBNodePtr>;
using BBNodeStack = std::stack<BBNodePtr>;
using BBDocumentPtr = std::shared_ptr<BBDocument>;

using ParameterMap = std::map<std::string, std::string>;

class BBNode : public std::enable_shared_from_this<BBNode>
{
    template<typename NewTypePtrT>
    NewTypePtrT cast(BBNodePtr node, bool bThrowOnFail)
    {
        if (node == nullptr && !bThrowOnFail)
        {
            return NewTypePtrT();
        }
        else if (node == nullptr)
        {
            throw std::invalid_argument("Cannot downcast BBNode, object is null");
        }

        NewTypePtrT newobj = std::dynamic_pointer_cast<typename NewTypePtrT::element_type, BBNode>(node);

        if (newobj == nullptr && bThrowOnFail)
        {
            throw std::invalid_argument("Cannot downcast, object is not correct type");
        }

        return newobj;
    }   

    template<typename NewTypePtrT>
    NewTypePtrT cast(BBNodePtr node, bool bThrowOnFail) const
    {
        if (node == nullptr && !bThrowOnFail)
        {
            return NewTypePtrT();
        }
        else if (node == nullptr)
        {
            throw std::invalid_argument("Cannot downcast, BBNode object is null");
        }

        NewTypePtrT newobj = std::dynamic_pointer_cast<typename NewTypePtrT::element_type, BBNode>(node);

        if (newobj == nullptr && bThrowOnFail)
        {
            throw std::invalid_argument("Cannot downcast, object is not correct type");
        }

        return newobj;
    }      
        
public:
    enum class NodeType
    {
        DOCUMENT,   
        ELEMENT,    // [b]bold[/b], [QUOTE], [QUOTE=Username;1234], [QUOTE user=Bob] 
        TEXT,       // plain text
        ATTRIBUTE
    };

    BBNode(NodeType nodeType, const std::string& name);
    virtual ~BBNode() = default;

    const std::string& getNodeName() const { return _name; }
    NodeType getNodeType() const { return _nodeType; }
    BBNodePtr getParent() const { return BBNodePtr(_parent); }

    const BBNodeList& getChildren() const { return _children; }

    virtual void appendChild(BBNodePtr node)
    {
        _children.push_back(node);
        node->_parent = shared_from_this();
    }

    template<typename NewTypePtrT>
	NewTypePtrT downCast(bool bThrowOnFail = true)
	{
		return cast<NewTypePtrT>(shared_from_this(), bThrowOnFail);
	}

    template<typename NewTypePtrT>
	NewTypePtrT downCast(bool bThrowOnFail = true) const
	{
		return cast<NewTypePtrT>(shared_from_this(), bThrowOnFail);
	}
  
protected:
    std::string     _name;
    NodeType        _nodeType;
    BBNodeWeakPtr   _parent;
    BBNodeList      _children;

    friend class BBText;
    friend class BBDocument;
    friend class BBElement;
};

class BBText : public BBNode
{
public:
    BBText(const std::string& value)
        : BBNode(BBNode::NodeType::TEXT, value)
    {
        // nothing to do
    }

    virtual ~BBText() = default;

    virtual const std::string getText() const { return _name; }

    void append(const std::string& text)
    {
        _name.append(text);
    }
};

class BBElement : public BBNode
{
public:
    enum ElementType
    {
        SIMPLE,     // [b]bold[/b], [code]print("hello")[/code]
        VALUE,      // [QUOTE=Username;12345]This is a quote[/QUOTE] (mostly used by vBulletin)
        PARAMETER,  // [QUOTE user=Bob userid=1234]This is a quote[/QUOTE]
        CLOSING     // [/b], [/code]
    };

    BBElement(const std::string& name, ElementType et = BBElement::SIMPLE)
        : BBNode(BBNode::NodeType::ELEMENT, name),
          _elementType(et)
    {
        // nothing to do
    }

    virtual ~BBElement() = default;

    const ElementType getElementType() const { return _elementType; }

    void setOrAddParameter(const std::string& key, const std::string& value, bool addIfNotExists = true)
    {
        _parameters.insert({key,value});
    }

    std::string getParameter(const std::string& key, bool bDoThrow = true)
    {
        if (_parameters.find(key) == _parameters.end() && bDoThrow)
        {
            throw std::invalid_argument("Undefine attribute '" + key + "'");
        }

        return _parameters.at(key);
    }

    const ParameterMap& getParameters() const { return _parameters; }

private:
    ElementType       _elementType = BBElement::SIMPLE;
    ParameterMap      _parameters;
};

class BBDocument : public BBNode
{
    // Private for a good reason - this object must be managed by std::shared_ptr,
    // otherwise parser crashes due to using `shared_from_this()`
    // See https://stackoverflow.com/questions/25628704/enable-shared-from-this-why-the-crash
    BBDocument()
        : BBNode(BBNode::NodeType::DOCUMENT, "#document")
    {
        // nothing to do
    }

	template <typename citerator>
    citerator parseText(citerator begin, citerator end)
    {
        auto endingChar = begin;

        for (auto it = begin; it != end; it++)
        {
            if (*it == '[')
            {
                endingChar = it;
                break;
            }
        }

        if (endingChar == begin)
        {
            endingChar = end;
        }

        newText(std::string(begin, endingChar));

        return endingChar;
    }

     template <typename citerator>
     citerator parseElementName(citerator begin, citerator end, std::string& buf)
     {
         auto start = begin;
         std::stringstream str;

         for (auto it = start; it != end; it++)
         {
             // RIGSOFRODS: also support singular [*] as list item.
             const char c = (char)*it;
            if (bbcpp::IsAlNum(c) || c == '*')
            {
                str << *it;
            }
            else
            {
                buf.assign(str.str());
                return it;
            }
         }

         return start;
     }

     template <typename citerator>
     citerator parseAttributeValue(citerator begin, citerator end, std::string& value)
     {
         auto start = begin;
         while (bbcpp::IsSpace(*start) && start != end)
         {
             start++;
         }

         if (start == end)
         {
             // we got to the end and there was nothing but spaces
             // so return our starting point so the caller can create
             // a text node with those spaces
             return end;
         }

         // RIGSOFRODS: Apparently XenForo allows [] inside alt text:
         // [ATTACH alt="screenshot_2018-07-16_23-35-33_1_tzC[1].png"]1612[/ATTACH]
         // .................................................^^^
         int internal_openbracket_stack = 0;

         std::stringstream temp;

         for (auto it = start; it != end; it++)
         {
             // RIGSOFRODS: removed the `IsAlNum()` and all color/url special character checks because:
             // * '_' was missing from the original code
             // * UTF-8 characters would cause havoc

             if (*it == '\"') // RIGSOFRODS: added support for quoted attribute values
             {
                 // Opening or closing quotemark - skip it.
                 continue;
             }
             else if (*it == '[') // RIGSOFRODS: tolerate [] inside attribute value, if matching
             {
                 internal_openbracket_stack++;
                 temp << *it;
             }
             else if (*it == ']')
             {
                 if (internal_openbracket_stack == 0)
                 {
                     value.assign(temp.str());
                     return it;
                 }
                 else
                 {
                     internal_openbracket_stack--;
                     temp << *it;
                 }
             }
             else
             {
                 temp << *it;
             }
         }

         // if we get here then we're at the end, so we return the starting
         // point so the callerd can create a text node
         return end;
     }

    template <typename citerator>
    citerator parseKey(citerator begin, citerator end, std::string& keyname)
    {
        auto start = begin;
        while (bbcpp::IsSpace(*start) && start != end)
        {
            start++;
        }

        if (start == end)
        {
            // we got to the end and there was nothing but spaces
            // so return our end point so the caller can create
            // a text node with those spaces
            return start;
        }

        std::stringstream temp;

        // TODO: need to handle spaces after the key name and before
        // the equal sign (ie. "[style color  =red]")
        for (auto it = start; it != end; it++)
        {
            if (bbcpp::IsAlNum(*it))
            {
                temp << *it;
            }
            else if (*it == '=')
            {
                keyname.assign(temp.str());
                return it;
            }
            else
            {
                // some invalid character, so return the point where
                // we stopped parsing
                return it;
            }
        }

        // if we get here then we're at the end, so we return the starting
        // point so the callerd can create a text node
        return end;
    }

    template <typename citerator>
    citerator parseKeyValuePairs(citerator begin, citerator end, ParameterMap& pairs)
    {
        auto current = begin;
        std::string tempKey;
        std::string tempVal;

        while (current != end)
        {
            current = parseKey(current, end, tempKey);
            if (tempKey.empty())
            {
                pairs.clear();
                return current;
            }

            if (*current != '=')
            {
                pairs.clear();
                return current;
            }

            current = std::next(current);
            current = parseAttributeValue(current, end, tempVal);

            if (tempKey.empty() || tempVal.empty())
            {
                pairs.clear();
                return current;
            }

            pairs.insert(std::make_pair(tempKey, tempVal));
            if (*current == ']')
            {
                // this is the only valid condition for key/value pairs so we do
                // not want to clear `pairs` like in the other cases
                return current;
            }
        }

        return end;
    }

    template <typename citerator>
    citerator parseElement(citerator begin, citerator end)
    {
        bool closingTag = false;

        // the first non-[ and non-/ character
        auto nameStart = std::next(begin);

        std::string elementName;

        // this might be a closing tag so mark it
        if (*nameStart == '/')
        {
            closingTag = true;
            nameStart = std::next(nameStart);
        }

        auto nameEnd = parseElementName(nameStart, end, elementName);

        // no valid name was found, so bail out
        if (elementName.empty())
        {
            newText(std::string{*begin});
            return nameEnd;
        }
        else if (nameEnd == end)
        {
            newText(std::string(begin,end));
            return end;
        }

        if (*nameEnd == ']')
        {
            // end of element
        }
        else if (*nameEnd == '=')
        {
            // possibly a QUOTE value element
            // possibly key-value pairs of a QUOTE
            ParameterMap pairs;
            
            auto kvEnd = parseKeyValuePairs(nameStart, end, pairs);
            if (pairs.size() == 0)
            {
                newText(std::string(begin, kvEnd));
                return kvEnd;
            }
            else
            {
                newKeyValueElement(elementName, pairs);
                // TODO: add 'pairs'
                return std::next(kvEnd);
            }
        }
        else if (*nameEnd == ' ')
        {
            // possibly key-value pairs of a QUOTE
            ParameterMap pairs;

            auto kvEnd = parseKeyValuePairs(nameEnd, end, pairs);
            if (pairs.size() == 0)
            {
                newText(std::string(begin, kvEnd));
                return kvEnd;
            }
            else
            {
                newKeyValueElement(elementName, pairs);
                // TODO: add 'pairs'
                return std::next(kvEnd);
            }
        }
        else
        {
            // some invalid char proceeded the element name, so it's not actually a
            // valid element, so create it as text and move on
            newText(std::string(begin,nameEnd));
            return nameEnd;
        }

        if (closingTag)
        {
            newClosingElement(elementName);
        }
        else
        {
            newElement(elementName);
        }

        return std::next(nameEnd);
    }

public: 
    static BBDocumentPtr create()
    {
        BBDocumentPtr doc = BBDocumentPtr(new BBDocument());
        return doc;
    }

    void load(const std::string& bbcode)
    {
        load(bbcode.begin(), bbcode.end());
    }

    template<class Iterator>
    void load(Iterator begin, Iterator end)
    {
        std::string buffer;
        auto bUnknownNodeType = true;
        auto current = begin;
        auto nodeType = BBNode::NodeType::TEXT;

        Iterator temp;

        while (current != end)
        {
            if (bUnknownNodeType)
            {
                if (*current == '[')
                {
                    nodeType = BBNode::NodeType::ELEMENT;
                    bUnknownNodeType = false;
                }
                else
                {
                    nodeType = BBNode::NodeType::TEXT;
                    bUnknownNodeType = false;
                }
            }
            
            if (!bUnknownNodeType)
            {
                switch (nodeType)
                {
                    default:
                        throw std::runtime_error("Unknown node type in BBDocument::load()");
                    break;

                    case BBNode::NodeType::TEXT:
                    {
                        current = parseText(current, end);
                        bUnknownNodeType = true;
                    }
                    break;

                    case BBNode::NodeType::ELEMENT:
                    {
                        temp  = parseElement(current, end);
                        if (temp == current)
                        {
                            // nothing was parsed, treat as text
                            nodeType = BBNode::NodeType::TEXT;
                            bUnknownNodeType = false;
                        }
                        else
                        {
                            current = temp;
                            bUnknownNodeType = true;
                        }
                    }
                    break;                    
                }
            }
        }
    }

private:
    BBNodeStack     _stack;

    BBText& newText(const std::string& text = std::string());
    BBElement& newElement(const std::string& name);
    BBElement& newClosingElement(const std::string& name);
    BBElement& newKeyValueElement(const std::string& name, const ParameterMap& pairs);
};

namespace
{

std::ostream& operator<<(std::ostream& os, const ParameterMap& params)
{
    bool first = true;
    os << "{ ";
    for (auto& p : params)
    {
        os << (first ? "" : ", ") << "{" << p.first << "=" << p.second << "}";
        if (first)
        {
            first = false;
        }
    }
    return (os << " }");
}

}


    
} // namespace
