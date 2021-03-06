require 'rd/rd2man-lib'
require 'rd-visitor-util'

module RD
  class RD2MANVisitor
    include RDVisitorUtil

    def apply_to_DocumentElement(element, content)
      content = content.join
      title = @title || guess_title
      <<"EOT"
.\\" DO NOT MODIFY THIS FILE! it was generated by rd2
.TH #{title} 1 "#{Time.now.strftime '%B %Y'}" "#{@source}" "#{@manual}"
#{content}
EOT
    end # "

    def apply_to_Headline(element, title)
      case element.level
      when 1
        @title, @source, @manual = title.join.split(/\s*\/\s*/, 3)
        @title = @title.upcase
        ""
      when 2
        ".SH #{title}\n"
      else
        ".SS #{title}\n"
      end
    end

    def apply_to_TextBlock(element, content)
      if RD::DescListItem === element.parent ||
          RD::ItemListItem === element.parent ||
          RD::EnumListItem === element.parent
        siblings = element.parent.children
        next_sibling = siblings[siblings.index(element) + 1]
        if RD::ItemList === next_sibling
          content.join.strip
        else
          content.join.strip + "\n"
        end
      else
        ".PP\n" + content.join.strip + "\n"
      end
    end

    def apply_to_ItemList(element, items)
      items.collect! do |x| x.sub(/\n\n/, "\n") end
      items = items.join(".IP\n.B\n\\(bu\n")  # "\\(bu" -> "" ?
      ".IP\n.B\n\\(bu\n" + items.strip + "\n"
    end

    def apply_to_ItemListItem(element, content)
      content.join("\n")
    end

    def apply_to_DescListItem(element, term, description)
      anchor = refer(element)
      term = term.join.strip
      if description.empty?
	".TP\n.fi\n.B\n#{term}\n"
      else
        %[.TP\n.fi\n.B\n#{term}\n#{description.join("\n")}].strip + "\n"
      end
    end

    def apply_to_RefToElement(element, content)
      content = content.join.sub(/^function#/, "")
      "\n.BR #{content.strip}\n"
    end

    def apply_to_Reference_with_RDLabel(element, content)
      "\n.BR #{content.join.sub(/\.rd(\.[a-z]{2})?\z/, '')}\n"
    end

    def apply_to_Var(element, content)
      "\n.I #{content.join.sub(/\./, '\\.')}\n"
    end

    def apply_to_Keyboard(element, content)
      "\n.B #{content.join.sub(/\./, '\\.')}\n"
    end

    def apply_to_StringElement(element)
      apply_to_String(remove_newline(element.content))
    end

    private
    def guess_title
      title = @filename || ARGF.filename || "Untitled"
      title = File.basename(title)
      title.sub(/\.rd$/i, '')
    end
  end
end
