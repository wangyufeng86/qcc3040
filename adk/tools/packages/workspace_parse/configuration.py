"""
%%fullcopyright(2018)
"""

property_xpath = "./property"
capability_xpath = "./group[@type='capability']"
deploy_xpath = "./deployProject"
name_attr_str = "name"
option_attr_str = "options"


class Configuration():
    'Representation of a configuration. Iterates over properties'
    def __init__(self, config_element=None):
        if config_element is not None:
            self.name = config_element.get(name_attr_str)
            self.properties = self._parse_properties(config_element)
            self.capabilities = self._parse_capabilities(config_element)
            self.options = self._parse_options(config_element)
            self.deploy = self._parse_deploy(config_element)
        else:
            self.name = 'None'
            self.properties = {}
            self.capabilities = {}
            self.options = []
            self.deploy = False

    def _parse_deploy(self, element):
        deploy_state = ""
        for deploy in element.findall(deploy_xpath):
            deploy_state = deploy.text
        # only returns the last instance found
        return deploy_state

    def _parse_properties(self, element):
        props = {}
        for prop in element.findall(property_xpath):
            property_name = prop.get(name_attr_str)
            property_value = prop.text if prop.text is not None else ""
            props[property_name] = property_value
        return props

    def _parse_capabilities(self, element):
        caps = {}
        for cap in element.findall(capability_xpath):
            caps[cap.get(name_attr_str)] = self._parse_properties(cap)
        return caps

    def _parse_options(self, element):
        options_str = element.get(option_attr_str)
        try:
            options = options_str.split('|')
        except AttributeError:
            options = []
        return options

    def __repr__(self):
        return self.name

    def __iter__(self):
        if self.properties:
            return iter(self.properties.values())
        return iter([])

    default_option_str = "default"
    clean_option_str = "clean"
    build_option_str = "build"
    run_option_str = "run"
    deploy_option_str = "deploy"
    finalBuild_option_str = "finalBuild"

    def is_cleanable(self):
        return self.clean_option_str in self.options

    def is_deployable(self):
        return self.deploy_option_str in self.options

    def is_buildable(self):
        return self.build_option_str in self.options

    def is_runnable(self):
        return self.run_option_str in self.options

    def is_default(self):
        return self.default_option_str in self.options

    def is_final_build(self):
        return self.finalBuild_option_str in self.options
