# -*- coding: utf-8 -*-
TSC.require "std/enable"

module Std

  # This class adds a climbing plant into a given box.
  # Use it like this:
  #
  #   c = ClimbingBox.new(UIDS[14], count: 4)
  #   c.attach
  #
  # This will create a climbing plant with 4 middle sprites (and a top
  # sprite) and attaches it to the box with UID 14. When that one gets
  # activated, the climbing plant will come out of it.
  #
  # Once you called #attach, Climbing boxes automatically save its
  # state to a savegame, so you don’t have to do that manually by
  # hooking into the level save and load events.
  class ClimbingBox

    # All the plant sprites.
    attr_reader :sprites
    # The underlying Box instance.
    attr_reader :box

    # Create a new climbing plant box.
    #
    # == Parameters
    # [box]
    #   A Box instance to attach the climbing plant to.
    # [opts]
    #   Hash for keyword arguments.
    #   [count]
    #     The number of sprites the plant shall have.
    #   [middle_graphic = "ground/green_1/kplant.png"]
    #     Middle graphic of the plant.
    #   [top_graphic = "ground/green_1/kplant_head.png"]
    #     Top graphci of the plant.
    def initialize(box, opts)
      opts[:count]          || raise(ArgumentError, "Required keyword argument :count missing!")
      opts[:middle_graphic] ||= "ground/green_1/kplant.png"
      opts[:top_graphic]    ||= "ground/green_1/kplant_head.png"

      @activated = false
      @box = box.kind_of?(Integer) ? UIDS[box] : box
      @sprites = []

      # Middle sprites
      1.upto(opts[:count]) do |i|
        s = Sprite.new(opts[:middle_graphic])
        box_x, box_y, box_w, box_h = @box.rect
        plant_x, plant_y, plant_w, plant_h = s.rect
        start_x = box_x + (box_w / ( 2 * box_x * plant_w)) # center plant tile on the box
        start_y = box_y - plant_y - (plant_h * i)
        s.massive_type = :climbable
        s.start_at(start_x, start_y)
        s.show

        @sprites << s
      end

      # Top sprite
      top_sprite = Sprite.new(opts[:top_graphic])
      top_sprite.start_at(@sprites.last.x, @sprites.last.y - top_sprite.rect[3])
      top_sprite.massive_type = :passive
      top_sprite.show
      @sprites << top_sprite

      # First hide all the sprites.
      @sprites.each{|sprite| sprite.disable}
    end

    # Attach the climbing plant to its box.
    def attach
      spritelist = @sprites.dup # We will empty this array on showing the plant
      @timer = nil
      @box.on_activate do
        Audio.play_sound("stomp_4.ogg")

        @timer = Timer.every(250) do
          spritelist.shift.enable

          @timer.stop! if spritelist.empty?
        end
      end

      # Automatic saving and loading.
      Level.on_save do |store|
        store["_ssl"] ||= {}
        store["_ssl"]["climbingboxes"] ||= {}
        store["_ssl"]["climbingboxes"][@box.uid] = @activated
      end

      Level.on_load do |store|
        if store["_ssl"] && store["_ssl"]["climbingboxes"] && store["_ssl"]["climbingboxes"][@box.uid]
          @sprites.each{|sprite| sprite.enable}
        end
      end
    end

    def activated?
      @activated
    end

    # Immediately show the entire climbing plant.
    def show_plant
      @sprites.each{|sprite| sprite.enable}
      @activated = true
    end

    # Immediately hide the entire climbing plant.
    def hide_plant
      @sprites.each{|sprite| sprite.disable}
      @activated = false
    end

  end

end
