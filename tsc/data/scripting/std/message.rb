module Std

  # Display a message to the user the same way a TextBox
  # does, i.e. a black box with white text inside appears
  # and the user has to press the Return key in order to
  # continue.
  #
  # Usage example:
  #
  #   m = Message.new("Lorem ipsum dolor sit amet...")
  #   m.activate
  #
  # You can reuse your message as often as you want. Also
  # note this is a subclass of the core TextBox class,
  # which especially allows you to change the message text
  # using the MessageBox#text= method.
  class Message < TextBox

    # Create a new Message with the given +text+, which
    # may be multiline.
    def initialize(text)
      super
      self.text = text
      self.massive_type = :passive
      hide
    end

    # Show the message to the user. This method ensures
    # the text always appears above Maryo.
    def activate
      # Ensure we always appear above the player
      self.start_at(Player.x, Player.y - 50)

      # Actual activation
      super

      #Now move the box away so it doesn't interact with the player (this won't move the text)
      self.warp(-100,100)
    end

  end

end
