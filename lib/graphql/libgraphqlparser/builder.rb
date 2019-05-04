module GraphQL
  module Libgraphqlparser
    # Keeps a stack of parse results, exposing the latest one.
    # The C parser can call methods on this object, assuming
    # they'll be applied to the right object.
    class Builder
      include GraphQL::Language

      attr_reader :document

      def initialize
        @ast_stack = []
      end

      def current
        @ast_stack.last
      end

      def begin_visit(node_class_name, assigns = {})
        #p "--> BEGIN #{node_class_name}"
        node = { name: node_class_name, assigns: assigns }.freeze
        @ast_stack.push(node)
        node
      end

      def end_visit
        node_value = @ast_stack.pop
        #p "--> END   #{node_value[:name]}"
        node = Nodes.const_get(node_value[:name]).new(node_value[:assigns])

        case node
        when Nodes::OperationDefinition, Nodes::FragmentDefinition
          current[:assigns][:definitions] ||= []
          current[:assigns][:definitions] << node
        when Nodes::VariableDefinition
          current[:assigns][:variables] ||= []
          current[:assigns][:variables] << node
        when Nodes::VariableIdentifier
          if current[:name] == 'VariableDefinition'
            current[:assigns][:name] = node.name
          else
            current[:assigns][:value] = node
          end
        when Nodes::Directive
          current[:assigns][:directives] ||= []
          current[:assigns][:directives] << node
        when Nodes::Argument
          current[:assigns][:arguments] ||= []
          current[:assigns][:arguments] << node
        when Nodes::InlineFragment, Nodes::FragmentSpread, Nodes::Field
          current[:assigns][:selections] ||= []
          current[:assigns][:selections] << node
        when Nodes::TypeName, Nodes::ListType, Nodes::NonNullType
          if current[:name] == 'ListType' || current[:name] == 'NonNullType'
            current[:assigns][:of_type] = node
          else
            current[:assigns][:type] = node
          end
        when Nodes::ListLiteral
          # mutability! 🎉
          current[:assigns][:value] = node.values
        when Nodes::NullValue
          current[:assigns][:value] = node
        when Nodes::InputObject
          current[:assigns][:value] = node
        when Nodes::Enum
          current[:assigns][:value] = node
        end

        node
      end

      def document_end_visit
        @document = end_visit
      end

      # This is for convenience from C
      def add_value(string_value)
        if current[:name] == 'VariableDefinition'
          current[:assigns][:default_value] = string_value
        elsif current[:name] == 'ListLiteral'
          current[:assigns][:values] ||= []
          current[:assigns][:values] << string_value
        else
          current[:assigns][:value] = string_value
        end
      end
    end
  end
end
