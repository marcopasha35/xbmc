/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>
#include <map>
#include <stack>
#include "utils/StdString.h"
#include "GUIInfoManager.h"

class CGUIListItem;

namespace INFO
{
/*!
 \ingroup info
 \brief Base class, wrapping boolean conditions and expressions
 */
class InfoBool
{
public:
  InfoBool(const CStdString &expression, int context)
    : m_value(false),
      m_context(context),
      m_listItemDependent(false),
      m_expression(expression),
      m_lastUpdate(0)
  {
  };

  virtual ~InfoBool() {};

  /*! \brief Get the value of this info bool
   This is called to update (if necessary) and fetch the value of the info bool
   \param time current time (used to test if we need to update yet)
   \param item the item used to evaluate the bool
   */
  inline bool Get(unsigned int time, const CGUIListItem *item = NULL)
  {
    if (item && m_listItemDependent)
      Update(item);
    else if (time - m_lastUpdate > 0)
    {
      Update(NULL);
      m_lastUpdate = time;
    }
    return m_value;
  }

  bool operator==(const InfoBool &right) const;

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(const CGUIListItem *item) {};

protected:

  bool m_value;                ///< current value
  int m_context;               ///< contextual information to go with the condition
  bool m_listItemDependent;    ///< do not cache if a listitem pointer is given
  friend unsigned int CGUIInfoManager::Register(const CStdString &, int, bool *);

private:
  CStdString m_expression;     ///< original expression
  unsigned int m_lastUpdate;   ///< last update time (to determine dirty status)
};

/*! \brief Class to wrap active boolean conditions
 */
class InfoSingle : public InfoBool
{
public:
  InfoSingle(const CStdString &condition, int context);
  virtual ~InfoSingle() {};

  virtual void Update(const CGUIListItem *item);
private:
  int m_condition;             ///< actual condition this represents
};

/*! \brief Class to wrap active boolean expressions
 */
class InfoExpression : public InfoBool
{
public:
  InfoExpression(const std::string &expression, int context);
  virtual ~InfoExpression() {};

  virtual void Update(const CGUIListItem *item);
private:
  typedef enum
  {
    OPERATOR_NONE  = 0,
    OPERATOR_LB,  // 1
    OPERATOR_RB,  // 2
    OPERATOR_OR,  // 3
    OPERATOR_AND, // 4
    OPERATOR_NOT, // 5
  } operator_t;

  typedef enum
  {
    NODE_LEAF,
    NODE_AND,
    NODE_OR,
  } node_type_t;

  // An abstract base class for nodes in the expression tree
  class InfoSubexpression
  {
  public:
    virtual ~InfoSubexpression(void) {}; // so we can destruct derived classes using a pointer to their base class
    virtual bool Evaluate(const CGUIListItem *item) = 0;
  };

  typedef boost::shared_ptr<InfoSubexpression> InfoSubexpressionPtr;

  // A leaf node in the expression tree
  class InfoLeaf : public InfoSubexpression
  {
  public:
    InfoLeaf(unsigned int info, bool invert) : m_info(info), m_invert(invert) {};
    virtual bool Evaluate(const CGUIListItem *item);
  private:
    unsigned int m_info;
    bool m_invert;
  };

  // A branch node in the expression tree
  class InfoAssociativeGroup : public InfoSubexpression
  {
  public:
    InfoAssociativeGroup(bool and_not_or, InfoSubexpressionPtr &left, InfoSubexpressionPtr &right);
    void AddChild(InfoSubexpressionPtr &child);
    void Merge(InfoAssociativeGroup *other);
    virtual bool Evaluate(const CGUIListItem *item);
  private:
    bool m_and_not_or;
    std::list<InfoSubexpressionPtr> m_children;
  };

  static operator_t GetOperator(char ch);
  static void OperatorPop(std::stack<operator_t> &operator_stack, bool &invert, std::stack<node_type_t> &node_types, std::stack<InfoSubexpressionPtr> &nodes);
  static void ProcessOperator(operator_t op, std::stack<operator_t> &operator_stack, bool &invert, std::stack<node_type_t> &node_types, std::stack<InfoSubexpressionPtr> &nodes);
  bool ProcessOperand(std::string &operand, bool invert, std::stack<node_type_t> &node_types, std::stack<InfoSubexpressionPtr> &nodes);
  bool Parse(const std::string &expression);
  InfoSubexpressionPtr m_expression_tree;
};

};
