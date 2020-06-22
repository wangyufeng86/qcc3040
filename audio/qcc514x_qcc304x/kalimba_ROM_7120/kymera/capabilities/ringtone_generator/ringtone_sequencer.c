/**
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
 * \defgroup ringtone_generator
 * \file  ringtone_sequencer.c
 * \ingroup  capabilities

 *  Ringtone generator sequencer
 *
 */
/****************************************************************************/

#include "ringtone_generator_cap.h"


static unsigned get_tone_input(RINGTONE_BUFFER *buf);
static bool next_tone(RINGTONE_GENERATOR_OP_DATA *rgop_data);

/**
* \brief    Initialise a ringtone sequence
*
* \param    rgop_data  The ringtone generator specific operator data
*/
void ringtone_init(RINGTONE_GENERATOR_OP_DATA *rgop_data)
{
    patch_fn_shared(ringtone_generator);

    TONE_SYNTH_INIT(&rgop_data->info);

    (void) ringtone_sequence_advance(rgop_data);
}


/**
* \brief    retrieve next tone parameter
*
* \param    *buf  index pointer to the tone buffer.
*
* \return    next parameter in the tone input buffer.
*/

static unsigned get_tone_input(RINGTONE_BUFFER *buf)
{
    unsigned param;

    param = (unsigned)(*(buf->index));

    buf->index++;

    return param;
}

/**
* \brief    Retrieve next tone definition on the link list and
*           free the previous one. If there is no more tone definition,
*           reset the list and indicate the tone is ended
*
* \param    rgop_data  The ringtone generator specific operator data
*
* \return   FALSE indicates reach the end of ringtone sequence, otherwise, return TRUE
*/
static bool next_tone(RINGTONE_GENERATOR_OP_DATA *rgop_data)
{
    RINGTONE_BUFFER *tone_input_buf;

    tone_input_buf = rgop_data->tone_list.head;
    if (tone_input_buf != NULL)
    {
        /* Get the next tone definition from the list */
        rgop_data->tone_list.head = tone_input_buf->next_tone;

        if(tone_input_buf->next_tone == NULL)
        {
           /* No more new tone need to be played */
           rgop_data->tone_end = TRUE;
           /* reset the list */
           rgop_data->tone_list.tail = NULL;
           
           pfree(tone_input_buf);
        }
        else
        {
            pfree(tone_input_buf);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * \brief Translation from ringtone_tone_type to tone_synth_tone_type.
 *        The values are the same, check that they are.
 */
static inline tone_synth_tone_type translate_tone_type(ringtone_tone_type rtt)
{
    COMPILE_TIME_ASSERT(tone_synth_tone_sine == ringtone_tone_sine,
                        ringtone_tone_sine__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_square == ringtone_tone_square,
                        ringtone_tone_square__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_saw == ringtone_tone_saw,
                        ringtone_tone_saw__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_triangle == ringtone_tone_triangle,
                        ringtone_tone_triangle__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_triangle2 == ringtone_tone_triangle2,
                        ringtone_tone_triangle2__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_clipped_sine == ringtone_tone_clipped_sine,
                        ringtone_tone_clipped_sine__needs_translation);
    COMPILE_TIME_ASSERT(tone_synth_tone_plucked == ringtone_tone_plucked,
                        ringtone_tone_plucked__needs_translation);

    return (tone_synth_tone_type) rtt;
}

/**
* \brief    Advance a sequence
*           All tone generation is based on 8000Hz, so for higher
*           frequencies, we scale relative to this. For example, to
*           generate samples at 16000Hz we increase the duration of
*           each tone by two, and half the size of the steps we take
*           through the tone.
*
* \param    rgop_data  The ringtone generator specific operator data
*
* \return   FALSE indicates reach the end of ringtone sequence, otherwise, return TRUE
*/
bool ringtone_sequence_advance(RINGTONE_GENERATOR_OP_DATA *rgop_data)
{
    L5_DBG_MSG("ringtone_sequence_advance \n");

    unsigned tone;

    if (rgop_data->tone_list.head == NULL)
    {
        return FALSE;
    }

    for(;;)
    {
        /* Check if we reach the end of the tone sent by the application */
        if(rgop_data->tone_list.head->tone_param_end > rgop_data->tone_list.head->index )
        {
            tone = get_tone_input(rgop_data->tone_list.head);
        }
        else
        {
             /* Move to the next tone if there is any */
             return(next_tone(rgop_data));
        }

        switch(tone & RINGTONE_SEQ_CONTROL_MASK)
        {
        case RINGTONE_SEQ_NOTE:
        {
            uint16 pitch = (tone & RINGTONE_SEQ_NOTE_PITCH_MASK) >> RINGTONE_SEQ_NOTE_PITCH_POS;
            uint16 length = (tone & RINGTONE_SEQ_NOTE_LENGTH_MASK) >> RINGTONE_SEQ_NOTE_LENGTH_POS;
            bool is_rest = (pitch == REST_NOTE_PITCH);
            bool is_tied = (tone & RINGTONE_SEQ_NOTE_TIE_MASK) != 0;
            TONE_SYNTH_SET_NOTE(&rgop_data->info, pitch, 1, length, is_rest, is_tied);
            return TRUE;
        }

        case RINGTONE_SEQ_CONTROL:

            switch(tone & RINGTONE_SEQ_CONTROL_COMMAND_MASK)
            {
                case RINGTONE_SEQ_END & RINGTONE_SEQ_CONTROL_COMMAND_MASK:

                    /* Move to the next tone if there is any */
                    return(next_tone(rgop_data));

                case RINGTONE_SEQ_TEMPO & RINGTONE_SEQ_CONTROL_COMMAND_MASK:
                    TONE_SYNTH_SET_TEMPO(&rgop_data->info, (tone & RINGTONE_SEQ_TEMPO_MASK));
                    return TRUE;

                case RINGTONE_SEQ_VOLUME & RINGTONE_SEQ_CONTROL_COMMAND_MASK:
                    tone_synth_set_volume(&rgop_data->info, (tone & RINGTONE_SEQ_VOLUME_MASK));
                    break;

                case RINGTONE_SEQ_TIMBRE & RINGTONE_SEQ_CONTROL_COMMAND_MASK:
                    tone_synth_set_tone_type(&rgop_data->info,
                                             translate_tone_type((ringtone_tone_type)(tone & RINGTONE_SEQ_TIMBRE_MASK)));
                    break;

                case RINGTONE_SEQ_DECAY & RINGTONE_SEQ_CONTROL_COMMAND_MASK:
                    tone_synth_set_decay(&rgop_data->info, (tone & RINGTONE_SEQ_DECAY_RATE_MASK));
                    break;
            }
        }
    }
}


