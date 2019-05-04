module GraphQL
  module Libgraphqlparser
    class PositionSource
      attr_reader :line, :col

      def initialize(line, col)
        @line = line
        @col = col
      end

      def line_and_column
        [@line, @col]
      end
    end
  end
end
