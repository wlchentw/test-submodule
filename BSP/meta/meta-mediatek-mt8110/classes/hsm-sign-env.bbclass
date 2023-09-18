python __anonymous () {
    hsm_env_str = ""

    hsm_support = d.getVar('HSM_SUPPORT', True)
    if hsm_support != 'yes':
        d.setVar('HSM_SIGN_TOOL','')
        d.setVar('HSM_SIGN_PARAM','')
        d.setVar('HSM_ENV', '')
        return
    hsm_env_str += "HSM_SUPPORT=yes "

    top_dir = d.getVar('TOPDIR',True)
    hsm_sign_tool_dir = top_dir+"/../src/devtools/hsmsigntool/"

    hsm_sign_tool = hsm_sign_tool_dir+"hsm_sign_tool"
    hsm_env_str += "HSM_SIGN_TOOL=\""+hsm_sign_tool+"\" "

    hsm_sign_tool_conf = d.getVar('HSM_SIGN_TOOL_CONF',True)
    if hsm_sign_tool_conf is None:
        hsm_sign_tool_conf = hsm_sign_tool_dir+"hsm_sign_tool.conf"
    hsm_env_str += "HSM_SIGN_TOOL_CONF=\""+hsm_sign_tool_conf+"\" "

    verified_key = d.getVar('VERIFIED_KEY')
    hsm_env_str += "VERIFIED_KEY=\""+verified_key+"\" "

    softhsm2_conf = d.getVar('SOFTHSM2_CONF',True)
    if softhsm2_conf is not None:
        hsm_env_str += "SOFTHSM2_CONF=\""+softhsm2_conf+"\" "

    d.setVar('HSM_SIGN_TOOL',hsm_sign_tool)
    d.setVar('HSM_SIGN_PARAM',' --signing_helper '+hsm_sign_tool)
    d.setVar('HSM_ENV', hsm_env_str)
}
