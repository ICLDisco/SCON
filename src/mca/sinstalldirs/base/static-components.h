/*
 * $HEADER$
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

extern const scon_mca_base_component_t mca_sinstalldirs_env_component;
extern const scon_mca_base_component_t mca_sinstalldirs_config_component;

const scon_mca_base_component_t *mca_sinstalldirs_base_static_components[] = {
  &mca_sinstalldirs_env_component, 
  &mca_sinstalldirs_config_component, 
  NULL
};

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

