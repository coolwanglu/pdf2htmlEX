// Basic node for the linked lists
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef BASICNODE_H__
#define BASICNODE_H__

#include <boost/intrusive/list.hpp>

namespace pdf2htmlEX {

enum class NodeType : char
{
    BasicNode
};

struct BasicListTag; // for boost::intrusive::list

/*
 * BasicNode is the common parent for all the nodes
 */
struct BasicNode
    : boost::intrusive::list_base_hook< tag<BasicListTag> >
{
    NodeType get_type () const { return type; }
private:
    NodeType type;
};

} // namespace pdf2htmlEX

#endif //BASICNODE_H__
