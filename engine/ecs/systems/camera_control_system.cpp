// ... (Dentro da função update) ...

// Itera sobre entidades que precisam de Tracking (Player) e tem os componentes de Movement e Input
for (auto const &pair : world.getComponents<Component::CameraTarget>()) {
    // ...
    
    // NOVO: Obter o InputComponent e o MovementComponent
    if (!world.hasComponent<Component::CameraInput>(entityID)) continue;
    Component::CameraInput &camInput = world.getComponent<Component::CameraInput>(entityID);
    Component::Movement &movement = world.getComponent<Component::Movement>(entityID);
    
    // 1. LÓGICA DE ROTAÇÃO DA CÂMERA (Antigo código do PlayerSystem)
    float rotationAmountDegrees = movement.rotationSpeed * dt;
    bool isRMBPressed = Engine::Input::InputManager::Get().IsRightMouseButtonPressed();

    // ROTAÇÃO A/D (Acoplado/Desacoplado)
    if (movement.isCameraOrbitModeActive) 
    {
        // Estado 2 (Desacoplado): Gira APENAS a câmera com A/D (Livre Look)
        if (Engine::Input::InputManager::Get().IsKeyPressed(GLFW_KEY_A)) 
            m_camera.setYaw(m_camera.getYaw() + rotationAmountDegrees); 
        if (Engine::Input::InputManager::Get().IsKeyPressed(GLFW_KEY_D)) 
            m_camera.setYaw(m_camera.getYaw() - rotationAmountDegrees);
    }
    // ROTAÇÃO RMB (Acoplado e Desacoplado): O MouseMoved Callback já cuida disso.

    // 2. ESCREVER VETORES DE INPUT (Delegar dados para o PlayerSystem)
    m_camera.setTarget(cameraFocusPoint); // <--- Lógica que você já tinha

    // A câmera DEVE ser OrbitCamera para ler Forward/Right
    if (auto *orbitCam = dynamic_cast<Engine::Camera::OrbitCamera *>(&m_camera))
    {
        // O CameraControlSystem LÊ a Câmera e ESCREVE no Componente
        camInput.forward = orbitCam->getForwardVector();
        camInput.right = orbitCam->getRightVector();
        camInput.yaw_degrees = orbitCam->getYaw(); // Salva o Yaw em graus para o PlayerSystem
    }
}