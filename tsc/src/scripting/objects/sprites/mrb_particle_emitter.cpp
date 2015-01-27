/***************************************************************************
 * mrb_particle_emitter.cpp
 *
 * Copyright © 2013-2014 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../../video/animation.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_particle_emitter.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/***************************************
 * Helpers
 ***************************************/

/*
 * Takes an MRuby Fixnum or Float and returns a C float for this.
 */
static mrb_float mrbnum2float(mrb_state* p_state, mrb_value obj)
{
    switch (mrb_type(obj)) {
    case MRB_TT_FIXNUM:
        return mrb_fixnum(obj); // Promote to float
    case MRB_TT_FLOAT:
        return mrb_float(obj);
    default:
        mrb_raisef(p_state, MRB_TYPE_ERROR(p_state), "Expected Fixnum or Float, got %s", mrb_obj_class(p_state, obj));
        return 0.0; // Not reached
    }
}

/*
 * Takes two floats and builds an MRuby range from them.
 */
static mrb_value range_from_rand_values(mrb_state* p_state, mrb_float middle, mrb_float rand)
{
    mrb_value beg = mrb_float_value(p_state, middle - rand);
    mrb_value end = mrb_float_value(p_state, middle + rand);

    return mrb_range_new(p_state, beg, end, 0); // beg..end (incl.)
}

/* Takes either a Fixnum, Float, or Range and calculates a
 * main parameter plus randomisation value from it.
 */
static void calculate_rand_values(mrb_state* p_state, mrb_value obj, mrb_float& value, mrb_float& rand)
{
    RRange* p_range = NULL;
    mrb_float beg, end;

    switch (mrb_type(obj)) {
    case MRB_TT_RANGE:
        p_range = mrb_range_ptr(obj);
        beg = mrbnum2float(p_state, p_range->edges->beg);
        end = mrbnum2float(p_state, p_range->edges->end);

        value = (end + beg) / 2.0; // mean
        rand = end - value;
        break;
    case MRB_TT_FIXNUM:
        value = mrb_fixnum(obj); // Promote to float
        rand = 0;
        break;
    case MRB_TT_FLOAT:
        value = mrb_float(obj);
        rand = 0;
        break;
    default:
        mrb_raisef(p_state, MRB_TYPE_ERROR(p_state), "Expected Numeric or Range, got %s", mrb_obj_classname(p_state, obj));
        return; // Not reached
    }
}

/**
 * Class: Animation
 *
 * Parent: [MovingSprite](movingsprite.html)
 * {: .superclass}
 *
 * TODO: Docs.
 */

/**
 * Method: Animation#time_to_live
 *
 *   time_to_live() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Time_to_Live(mrb_state* p_state,  mrb_value self)
{
    cAnimation* p_ani = Get_Data_Ptr<cAnimation>(p_state, self);
    return range_from_rand_values(p_state, p_ani->m_time_to_live, p_ani->m_time_to_live_rand);
}

/**
 * Method: Animation#time_to_live=
 *
 *   time_to_live=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Time_to_Live(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float time, rand;
    calculate_rand_values(p_state, obj, time, rand);

    cAnimation* p_ani = Get_Data_Ptr<cAnimation>(p_state, self);
    p_ani->Set_Time_to_Live(time, rand);

    return mrb_nil_value();
}

/**
 * Class: ParticleEmitter
 *
 * Parent: [Animation](animation.html)
 * {: .superclass}
 *
 * The _ParticleEmitter_ is one of the most complex classes in the
 * API. If you’re not familiar with using regular particle emitters from
 * the normal TSC editor, you probably want to go there first and
 * experiment with them, because everything you need to adjust there
 * needs to be adjusted for dynamically created particle emitters as
 * well--with the difference that in the editor you have a nice UI
 * assisting you, whereelse for the dynamically created particle emitters
 * you have to _know_ what to set and what not to set. Particle emitters
 * can be quite hairy beasts, so I recommend you to often load your level
 * and test what your particle emitter will look like, e.g. by
 * registering for Alex’s _Jump_ event.
 *
 * Particle emitters can be used in two ways: The usual way, which means
 * periodically issueing a defined number of particles at a time. Such a
 * particle emitter can be created by setting the [emitter’s time to
 * live](#emittertimetolive-1) on that particle emitter. If you want more
 * finergrained control about what is going on, you can ignore that
 * setting and call the [#emit](#emit) method directly. Each call to that
 * method will cause the particle emitter to exactly once emit particles
 * according to its configuration.
 *
 * A good number of setter methods accept a Range instead of a singular
 * fixed parameter, which allows you to specify a range of valid values
 * for a given option. For instance, the [#time_to_live=](animation.html#timetolive-1)
 * methods allows you to define the ilfespan of particles emitted at a
 * time. Instead of setting this to a single, definite value (which you
 * can do if you want to), you can configure it to a Range like 1..2,
 * which means that the TTL will regularyly be 1.5, but at minimum 1.0
 * and at maximum 2.0, so that the particles emitted all are a little
 * different.
 *
 * Note that, in contrast to all other objects in TSC, it is possible to
 * set a particle emitter’s [Z coordinate](#z), making it possible to
 * appear in front of Alex or other sprites.
 *
 * Also note that `ParticleEmitter` is not a subclass of `Sprite` (the
 * particle emitter doesn’t show up on the screen itself, just its
 * emitted particles) and the methods defined there don’t apply here
 * therefore.
 *
 * == Example
 *
 * The following example shows how to create a particle emitter from
 * the scripting API. The object with UID 14 is assumed to be a box
 * you can jump against.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ruby
 * UIDS[14].on_activate do
 *   part = ParticleEmitter.new(100, -100, 100, 100)
 *   part.quota = 4
 *   part.image_filename = "animation/particles/snowflake_1.png"
 *   part.emitter_time_to_live = -1
 *   part.show
 * end
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * When the player jumps against the box, the game will create a particle
 * emitter at position (100|-100) with a size of 100x100 (all pixel values).
 * The emitter will emit snowflakes as per the image file in
 * `animation/particles/snowflake_1.png` below the `pixmaps/` directory, four
 * flakes at a time. It will do so infinitely (-1).
*/

/**
 * Method: ParticleEmitter::new
 *
 *   new( x, y [, width [, height ] ] ) → a_particle_emitter
 *
 * Creates a new particle emitter. It won’t emit particles by default,
 * you first have to adjust the emitter with the various setter methods,
 * and when you’ve done this you can either call [#emit](#emit) which will
 * gives you absolute control over each emitted particle, or use
 * [#emitter_time_to_live=](#emittertimetolive-1) to make the emitter
 * emit particles automatically.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    mrb_float x, y;
    mrb_float width = 0.0f;
    mrb_float height = 0.0f;

    mrb_get_args(p_state, "ff|ff", &x, &y, &width, &height);

    cParticle_Emitter* p_emitter = new cParticle_Emitter(pActive_Level->m_sprite_manager);
    p_emitter->Set_Emitter_Rect(x, y, width, height);

    // This is a generated object
    p_emitter->Set_Spawned(true);

    /* Let TSC manage the memory, as with all TSC level objects
     * This has some side effects: Although we create the object
     * as per the code above, we don’t have to clean it up, this
     * is done automatically by TSC when the particle emitter has
     * finished. It also means that the DATA pointer inside the
     * MRuby object gets invalid in the instant TSC cleans the
     * pointer. Not sure if we really need to cater for this, although
     * it of course leads to segfaults if such an MRuby object is
     * used...
     */
    pActive_Animation_Manager->Add(p_emitter);

    DATA_PTR(self) = p_emitter;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    return self;
}

/**
 * Method: ParticleEmitter#z=
 *
 *   z=( val )
 *
 * TODO: Docs.
 */
static mrb_value Set_Z(mrb_state* p_state,  mrb_value self)
{
    mrb_float z;
    mrb_get_args(p_state, "f", &z);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Pos_Z(z);

    return mrb_float_value(p_state, z);
}

/**
 * Method: ParticleEmitter#image_filename
 *
 *   image_filename() → a_string
 *
 * The filename of the image of the particles emitted by this emitter,
 * relative to the `pixmaps/` directory.
 */
static mrb_value Get_Image_Filename(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return mrb_str_new_cstr(p_state, path_to_utf8(p_emitter->m_image_filename).c_str());
}

/**
 * Method: ParticleEmitter#image_filename=
 *
 *   image_filename=( path )
 *
 * Sets the filename of the image to use for particles generated by the
 * particle emitter, relative to the `pixmaps/` directory.
 */
static mrb_value Set_Image_Filename(mrb_state* p_state,  mrb_value self)
{
    char* str = NULL;
    mrb_get_args(p_state, "z", &str);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Image_Filename(utf8_to_path(str));

    return mrb_str_new_cstr(p_state, str);
}

/**
 * Method: ParticleEmitter#scale
 *
 *   scale() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Scale(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_size_scale, p_emitter->m_size_scale_rand);
}

/**
 * Method: ParticleEmitter#speed
 *
 *   speed() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Speed(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_vel, p_emitter->m_vel_rand);
}

/**
 * Method: ParticleEmitter#emitter_time_to_live
 *
 *   emitter_time_to_live() → a_float
 *
 * The number of seconds the emitter will emit particles. This is independant
 * from the TTL of the respective particles.
 */
static mrb_value Get_Emitter_Time_To_Live(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return mrb_float_value(p_state, p_emitter->m_emitter_time_to_live);
}

/**
 * Method: ParticleEmitter#quota
 *
 *   quota() → an_integer
 *
 * TODO: Docs.
 */
static mrb_value Get_Quota(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return mrb_fixnum_value(p_emitter->m_emitter_quota);
}

/**
 * Method: ParticleEmitter#gravity_x
 *
 *   gravity_x() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Gravity_X(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_gravity_x, p_emitter->m_gravity_x_rand);
}

/**
 * Method: ParticleEmitter#gravity_y
 *
 *   gravity_y() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Gravity_Y(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_gravity_y, p_emitter->m_gravity_y_rand);
}

/**
 * Method: ParticleEmitter#const_rotation_x
 *
 *   const_rotation_x() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Const_Rotation_X(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_const_rot_x, p_emitter->m_const_rot_x_rand);
}

/**
 * Method: ParticleEmitter#const_rotation_y
 *
 *   const_rotation_y() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Const_Rotation_Y(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_const_rot_y, p_emitter->m_const_rot_y_rand);
}

/**
 * Method: ParticleEmitter#const_rotation_z
 *
 *   const_rotation_z() → a_range
 *
 * TODO: Docs.
 */
static mrb_value Get_Const_Rotation_Z(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    return range_from_rand_values(p_state, p_emitter->m_const_rot_z, p_emitter->m_const_rot_z_rand);
}

/**
 * Method: ParticleEmitter#inspect
 *
 *   inspect() → a_string
 *
 * Human-readable description.
 */
static mrb_value Inspect(mrb_state* p_state,  mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    char buffer[256];

    int num = sprintf(buffer,
                      "#<%s pos=(%.2f|%.2f) ETTL=%.2fs>",
                      mrb_obj_classname(p_state, self),
                      p_emitter->m_pos_x,
                      p_emitter->m_pos_y,
                      p_emitter->m_emitter_time_to_live);

    if (num < 0)
        mrb_raisef(p_state, MRB_RUNTIME_ERROR(p_state), "Couldn't format string, sprintf() returned %d", num);

    return mrb_str_new(p_state, buffer, num);
}

/**
 * Method: ParticleEmitter#scale=
 *
 *   scale=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Scale(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Scale(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#speed=
 *
 *   speed=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Speed(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Speed(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#emitter_time_to_live=
 *
 *   emitter_time_to_live=( value )
 *
 * Sets the emitter’s time to live (TTL). After the specified number of
 * seconds has passed, the emitter will stop emitting particles. This
 * value is independant from the TTL of the respective particles, which
 * can be set with the `Animation#time_to_live=` method.
 */
static mrb_value Set_Emitter_Time_To_Live(mrb_state* p_state,  mrb_value self)
{
    mrb_int value;
    mrb_get_args(p_state, "i", &value);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Emitter_Time_to_Live(value);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#quota=
 *
 *   quota=( value )
 *
 * Sets the number of particles emitted at one time.
 */
static mrb_value Set_Quota(mrb_state* p_state, mrb_value self)
{
    mrb_int quota;
    mrb_get_args(p_state, "i", &quota);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Quota(quota);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#gravity_x=
 *
 *   gravity_x=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Gravity_X(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Horizontal_Gravity(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#gravity_y=
 *
 *   gravity_y=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Gravity_Y(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Vertical_Gravity(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#const_rotation_x=
 *
 *   const_rotation_x=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Const_Rotation_X(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Const_Rotation_X(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#const_rotation_y=
 *
 *   const_rotation_y=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Const_Rotation_Y(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Const_Rotation_Y(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#const_rotation_z=
 *
 *   const_rotation_z=( range )
 *
 * TODO: Docs.
 */
static mrb_value Set_Const_Rotation_Z(mrb_state* p_state,  mrb_value self)
{
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);

    mrb_float value, rand;
    calculate_rand_values(p_state, obj, value, rand);

    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Set_Const_Rotation_Z(value, rand);

    return mrb_nil_value();
}

/**
 * Method: ParticleEmitter#emit
 *
 *   emit()
 *
 * Emit a single particle (or multiple ones if the [#quota](#quota) is
 * set accordingly). Usually you want to use
 * [#emitter_time_to_live=](#emittertimetolive1) to make the
 * emitter act automatically, but this method allows to retain full
 * control about the particle emitter.
*/
static mrb_value Emit(mrb_state* p_state, mrb_value self)
{
    cParticle_Emitter* p_emitter = Get_Data_Ptr<cParticle_Emitter>(p_state, self);
    p_emitter->Emit();
    return mrb_nil_value();
}

/***************************************
 * Binding
 ***************************************/

void TSC::Scripting::Init_ParticleEmitter(mrb_state* p_state)
{
    struct RClass* p_rcAnimation = mrb_define_class(p_state, "Animation", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcAnimation, MRB_TT_DATA);

    // Methods
    mrb_define_method(p_state, p_rcAnimation, "time_to_live", Get_Time_to_Live, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcAnimation, "time_to_live=", Set_Time_to_Live, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_method(p_state, p_rcAnimation, "z=", Set_Z, MRB_ARGS_REQ(1));

    struct RClass* p_rcParticleEmitter = mrb_define_class(p_state, "ParticleEmitter", p_rcAnimation);
    MRB_SET_INSTANCE_TT(p_rcParticleEmitter, MRB_TT_DATA);

    // Methods
    mrb_define_method(p_state, p_rcParticleEmitter, "initialize", Initialize, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(2));
    mrb_define_method(p_state, p_rcParticleEmitter, "inspect", Inspect, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "image_filename", Get_Image_Filename, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "image_filename=", Set_Image_Filename, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "scale", Get_Scale, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "emitter_time_to_live", Get_Emitter_Time_To_Live, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "quota", Get_Quota, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "gravity_x", Get_Gravity_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "gravity_y", Get_Gravity_Y, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_x", Get_Const_Rotation_X, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_y", Get_Const_Rotation_Y, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_Z", Get_Const_Rotation_Z, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcParticleEmitter, "scale=", Set_Scale, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "emitter_time_to_live=", Set_Emitter_Time_To_Live, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "quota=", Set_Quota, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "gravity_x=", Set_Gravity_X, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "gravity_y=", Set_Gravity_Y, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_x=", Set_Const_Rotation_X, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_y=", Set_Const_Rotation_Y, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "const_rotation_z=", Set_Const_Rotation_Z, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcParticleEmitter, "emit", Emit, MRB_ARGS_NONE());
}
