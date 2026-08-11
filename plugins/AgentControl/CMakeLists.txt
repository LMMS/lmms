INCLUDE(BuildPlugin)

BUILD_PLUGIN(AgentControl
    AgentControl.cpp
    AgentControl.h
    AgentControlView.cpp
    AgentControlView.h
    agent_infra.cpp
    agent_project.cpp
    agent_arrangement.cpp
    agent_notes.cpp
    agent_pattern.cpp
    agent_sample.cpp
    agent_instrument.cpp
    agent_sound.cpp
    agent_effect.cpp
    agent_mixer.cpp
    agent_automation.cpp
    agent_render.cpp
    agent_record.cpp
    agent_misc.cpp
    MOCFILES AgentControl.h AgentControlView.h
)

target_link_libraries(AgentControl Qt${QT_VERSION_MAJOR}::Network)
