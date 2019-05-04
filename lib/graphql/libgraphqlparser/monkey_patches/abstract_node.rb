module GraphQL
  module Language
    module Nodes
      class ListLiteral < AbstractNode
        attr_reader :values

        def initialize_node(options={})
          @values = options[:values]
        end
      end
    end
  end
end
