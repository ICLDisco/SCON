/*
 * $HEADER$
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

extern const scon_mca_base_component_t mca_sdl_sdlopen_component;

const scon_mca_base_component_t *mca_sdl_base_static_components[] = {
  &mca_sdl_sdlopen_component, 
  NULL
};

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

