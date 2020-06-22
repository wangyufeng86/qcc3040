#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__
#include <app/vm/vm_if.h>
#include <sink_.h>
#include <source_.h>
#include <transform_.h>

/*! file  @brief Transform data between sources and sinks */

#if TRAPSET_HIDDONGLE

/**
 *  \brief Generic HID transform supporting the following devices a) Boot mode mouse b)
 *  Report mode mouse c) Boot mode keyboard d) Report mode keyboard
 *   
 *  \param source The Source data will be taken from. 
 *  \param sink The Sink data will be written to.
 *  \return An already started transform on success, or zero on failure.
 * 
 * \ingroup trapset_hiddongle
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Transform TransformHid(Source source, Sink sink);
#endif /* TRAPSET_HIDDONGLE */
#if TRAPSET_KALIMBA

/**
 *  \brief Packs Audio frames from the DSP into RTP packets. This trap attaches RTP
 *  stamping for the Audio packets arriving from DSP. It configures default codec
 *  type as SBC and also sets payload header size for SBC. The sequence of
 *  function calls in a VM Application which is acting as an encoder would be: 1.
 *  Call the TransformRtpEncode trap. 2. Using the TransformConfigure trap
 *  configure the required parameters: 2.1 Codec type (APTX / SBC / ATRAC / MP3 /
 *  AAC) 2.2 Manage Timing (Yes/No) 2.3 Payload header size 2.4 SCMS Enable
 *  (Yes/No) 2.5 SCMS Bits 2.6 Frame period 2.7 Packet size 3. Call the
 *  TransformStart trap.
 *   
 *  \param source The media Source. 
 *  \param sink The sink receiving the Audio Digital stream (typically corresponding to a
 *  Kalimba port).
 *  \return The transform if successful, or zero on failure.
 * 
 * \ingroup trapset_kalimba
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Transform TransformRtpEncode(Source source, Sink sink);

/**
 *  \brief Unpacks Audio frames from Audio-RTP packets. It configures default codec type
 *  as SBC and also sets payload header size for SBC.
 *   
 *  \param source The source containing the Audio Digital stream (typically corresponding to a
 *  Kalimba port). 
 *  \param sink The media Sink.
 *  \return The transform if successful, or zero on failure.
 * 
 * \ingroup trapset_kalimba
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Transform TransformRtpDecode(Source source, Sink sink);
#endif /* TRAPSET_KALIMBA */
#if TRAPSET_STREAM

/**
 *  \brief Start a transform; newly created transforms must be started.
 *   
 *   @note
 *   Application shouldn't call this function for transforms created via 
 *   @a StreamConnect() call as those transforms are started implicitly.
 *  \param transform The transform to start.
 *  \return FALSE on failure, TRUE on success.
 * 
 * \ingroup trapset_stream
 */
bool TransformStart(Transform transform);

/**
 *  \brief Stop a transform,
 *   
 *   @note
 *   Application shouldn't call this function for transforms created 
 *   via @a StreamConnect() call. To stop data flow for such transforms, 
 *   application would typically call @a TransformDisconnect().
 *  \param transform The transform to stop.
 *  \return FALSE on failure, TRUE on success.
 * 
 * \ingroup trapset_stream
 */
bool TransformStop(Transform transform);

/**
 *  \brief Disconnect and destroy a transform. 
 *   
 *   The transform can no longer be used after successful disconnecting of the 
 *   transform.
 *  \param transform The transform to destroy.
 *  \return TRUE on success, FALSE on failure.
 * 
 * \ingroup trapset_stream
 */
bool TransformDisconnect(Transform transform);

/**
 *  \brief Report if any traffic has been handled by this transform.
 *   Reads and clears a bit that reports any activity on a
 *   transform. This can be used to detect activity on connect streams.
 *   
 *  \param transform The transform to query.
 *  \return TRUE if the transform exists and has processed data, FALSE otherwise. @note If
 *  the transform is connected between stream and an operator then this trap
 *  reports traffic by the transform. But, if the transform is connected between
 *  two operators inside the DSP then the trap would return FALSE.
 * 
 * \ingroup trapset_stream
 */
bool TransformPollTraffic(Transform transform);

/**
 *  \brief Find the transform connected to a source.
 *   
 *  \param source The source to look for.
 *  \return The transform connected to the specified source, or zero if no transform or
 *  connection is active.
 * 
 * \ingroup trapset_stream
 */
Transform TransformFromSource(Source source);

/**
 *  \brief Find the transform connected to a sink.
 *   
 *  \param sink The sink to look for.
 *  \return The transform connected to the specified sink, or zero if no transform or
 *  connection is active.
 * 
 * \ingroup trapset_stream
 */
Transform TransformFromSink(Sink sink);

/**
 *  \brief Configure parameters associated with a transform. 
 *  \param transform The transform to configure. 
 *  \param key Valid values depend on the transform. 
 *  \param value Valid values depend on the transform. 
 *  \return Returns FALSE if the key was unrecognised, or if the value was out of bounds.
 * 
 * \ingroup trapset_stream
 */
bool TransformConfigure(Transform transform, vm_transform_config_key key, uint16 value);

/**
 *  \brief Create a transform between the specified source and sink. 
 *   
 *   Copies data in chunks.
 *  \param source The Source to use in the transform. 
 *  \param sink The Sink to use in the transform.
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 */
Transform TransformChunk(Source source, Sink sink);

/**
 *  \brief Create a transform between the specified source and sink. 
 *   
 *   Removes bytes from start and end of packets.
 *  \param source The Source to use in the transform. 
 *  \param sink The Sink to use in the transform.
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 */
Transform TransformSlice(Source source, Sink sink);

/**
 *  \brief Create an ADPCM decode transform between source and sink 
 *  \param source The source containing ADPCM encoded data 
 *  \param sink  The destination sink 
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Transform TransformAdpcmDecode(Source source, Sink sink);

/**
 *  \brief Create a packetising transform between the specified source and sink.
 *  The packetising transform loads audio frames into L2CAP packets (or vice
 *  versa). The packet format is
 * proprietary (TWS+), based on RTP. Various configuration is supported on the
 *  master (audio->L2CAP) side
 * to limit the size and timing of packets, as well as the codec and packet format.
 * @note
 * The audio endpoint must be connected to an operator capable of handling
 *  unexpected disconnections of the stream while the chain is active (e.g.
 *  switched
 * passthrough consumer or RTP decode), in order to allow the firmware to stop the
 *  audio service in the case of a link loss. Such an operator
 * must be present both on the input of the packetiser (on the forwarding device)
 *  and the output of the depacketizer (on the receiving device).
 *         
 *  \param source The source to use in the transform. 
 *  \param sink The sink to use in the transform.
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 */
Transform TransformPacketise(Source source, Sink sink);

/**
 *  \brief Create a clock domain converter transform between the specified source and
 *  sink. 
 *             The transform converts the 32 bits timing information present
 *             in the source buffer to sink's clock domain before writing
 *             to sink buffer.
 *             @note
 *             The transform will support any source and sink stream of application
 *             subsystem. The conversion from source clock domain to sink clock
 *             domain happens based on the timing offset received either from
 *             source or sink or both.
 *             For instance, if source is a file stream and sink is an L2cap stream
 *             then system time present in file source data would be converted
 *             based on the wallclock offset details provided by L2cap sink.
 *             @note
 *             Transform would work based on following configurations:
 *             1) VM_TRANSFORM_CLK_CONVERT_START_OFFSET
 *             2) VM_TRANSFORM_CLK_CONVERT_REPETITION_OFFSET
 *             3) VM_TRANSFORM_CLK_CONVERT_NUM_REPETITIONS
 *           
 *  \param source The source to use in the transform. 
 *  \param sink The sink to use in the transform.
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 */
Transform TransformConvertClock(Source source, Sink sink);

/**
 *  \brief Create a hash transform between specified source and sink streams. 
 *             Transform will calculate hash of source data packet and write lower
 *             16 bits of hash to configurable position in sink stream buffer.
 *             It can also be configured to prepend an RTP header before writing
 *             to sink stream buffer.
 *             @note
 *             Following configurations are required for functioning of this
 *             transform:
 *             1) VM_TRANSFORM_HASH_PREFIX_RTP_HEADER
 *             2) VM_TRANSFORM_HASH_RTP_PAYLOAD_TYPE
 *             3) VM_TRANSFORM_HASH_RTP_SSRC_LOWER
 *             4) VM_TRANSFORM_HASH_RTP_SSRC_UPPER
 *             5) VM_TRANSFORM_HASH_SOURCE_OFFSET
 *             6) VM_TRANSFORM_HASH_SOURCE_SIZE
 *             7) VM_TRANSFORM_HASH_SOURCE_MODIFY_OFFSET
 *             @note
 *             The transform can connect any source and sink including audio or
 *             operator streams except the case when both source and sink are
 *  either
 *             audio or operator stream.
 *           
 *  \param source The source to use in the transform. 
 *  \param sink The sink to use in the transform.
 *  \return 0 on failure, otherwise the transform.
 * 
 * \ingroup trapset_stream
 */
Transform TransformHash(Source source, Sink sink);
#endif /* TRAPSET_STREAM */
#endif
