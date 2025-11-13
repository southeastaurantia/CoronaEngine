#pragma once

#include <ktm/ktm.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Corona {
class Model;

namespace API {


// ============================================================================
// Geometry: 作为所有组件的锚点，存储位置/旋转/缩放和模型数据
// ============================================================================
class Geometry {
   public:
    Geometry();
    explicit Geometry(const std::string& model_path);
    virtual ~Geometry();

    // Transform operations
    void set_position(const std::array<float, 3>& position);
    void set_rotation(const std::array<float, 3>& euler);
    void set_scale(const std::array<float, 3>& size);

    [[nodiscard]] std::array<float, 3> get_position() const;
    [[nodiscard]] std::array<float, 3> get_rotation() const;
    [[nodiscard]] std::array<float, 3> get_scale() const;

    // Model management
    void load_model(const std::string& path);
    [[nodiscard]] std::uintptr_t get_model_handle() const { return model_handle_; }
    [[nodiscard]] std::uintptr_t get_transform_handle() const { return transform_handle_; }
    [[nodiscard]] std::uintptr_t get_bounding_handle() const { return bounding_handle_; }
    [[nodiscard]] std::uintptr_t get_device_handle() const { return device_handle_; }

   private:
    // Internal data (using ktm types for consistency with SharedDataHub)
    std::array<float, 3> position_{};
    std::array<float, 3> scale_{};
    std::array<float, 3> rotation_{};

    // SharedDataHub handles
    std::uintptr_t model_handle_{};
    std::uintptr_t transform_handle_{};
    std::uintptr_t bounding_handle_{};
    std::uintptr_t device_handle_{};
};

// ============================================================================
// Mechanics: 物理/力学组件，依赖 Geometry
// ============================================================================
class Mechanics {
   public:
    explicit Mechanics(Geometry& geo);
    virtual ~Mechanics();

    // Geometry access
    [[nodiscard]] const Geometry* get_geometry() const { return geometry_; }
    [[nodiscard]] Geometry* get_geometry() { return geometry_; }
    [[nodiscard]] bool is_bound() const { return geometry_ != nullptr; }

    // Physics operations (直接操作绑定的 Geometry)
    void move(const std::array<float, 3>& direction);
    void rotate(const std::array<float, 3>& euler);
    void scale(const std::array<float, 3>& size);
    void set_mass(float mass);
    [[nodiscard]] float get_mass() const { return mass_; }

   private:
    Geometry* geometry_;
    float mass_{70.0f};
};

// ============================================================================
// Optics: 光学/渲染组件，依赖 Geometry
// ============================================================================
class Optics {
   public:
    explicit Optics(Geometry& geo);
    virtual ~Optics();

    // Geometry access
    [[nodiscard]] const Geometry* get_geometry() const { return geometry_; }
    [[nodiscard]] Geometry* get_geometry() { return geometry_; }
    [[nodiscard]] bool is_bound() const { return geometry_ != nullptr; }

    // Rendering properties
    void set_material(const std::string& material_name);
    void set_visibility(bool visible);
    [[nodiscard]] const std::string& get_material() const { return material_name_; }
    [[nodiscard]] bool is_visible() const { return visible_; }

   private:
    Geometry* geometry_;
    std::string material_name_;
    bool visible_{true};
};

// ============================================================================
// Acoustics: 声学组件，依赖 Geometry
// ============================================================================
class Acoustics {
   public:
    explicit Acoustics(Geometry& geo);
    virtual ~Acoustics();

    // Geometry access
    [[nodiscard]] const Geometry* get_geometry() const { return geometry_; }
    [[nodiscard]] Geometry* get_geometry() { return geometry_; }
    [[nodiscard]] bool is_bound() const { return geometry_ != nullptr; }

    // Audio properties
    void play_sound(const std::string& sound_path);
    void set_volume(float volume);
    [[nodiscard]] float get_volume() const { return volume_; }

   private:
    Geometry* geometry_;
    float volume_{1.0f};
};

// ============================================================================
// Kinematics: 运动学/动画组件，依赖 Geometry
// ============================================================================
class Kinematics {
   public:
    explicit Kinematics(Geometry& geo);
    virtual ~Kinematics();

    // Geometry access
    [[nodiscard]] const Geometry* get_geometry() const { return geometry_; }
    [[nodiscard]] Geometry* get_geometry() { return geometry_; }
    [[nodiscard]] bool is_bound() const { return geometry_ != nullptr; }

    // Animation control
    void set_animation(std::uint32_t animation_index);
    void play_animation(float speed = 1.0f);
    void stop_animation();

    [[nodiscard]] std::uintptr_t get_animation_handle() const { return animation_handle_; }
    [[nodiscard]] std::uint32_t get_animation_index() const { return animation_index_; }
    [[nodiscard]] float get_current_time() const { return current_time_; }
    [[nodiscard]] bool is_active() const { return active_; }

   private:
    Geometry* geometry_;
    std::uintptr_t animation_handle_{};
    std::uint32_t animation_index_{0};
    float current_time_{0.0f};
    float speed_{1.0f};
    bool active_{false};
};

// ============================================================================
// Actor: OOP 风格的实体类，支持多套组件和多个 Geometry
// ============================================================================
class Actor {
   public:
    Actor();
    virtual ~Actor();

    // ========== Geometry 管理（支持多个） ==========
    std::size_t add_geometry(const Geometry& geo);
    std::size_t add_geometry(Geometry&& geo);
    void remove_geometry(std::size_t index);

    [[nodiscard]] Geometry* get_geometry(std::size_t index);
    [[nodiscard]] const Geometry* get_geometry(std::size_t index) const;
    [[nodiscard]] std::size_t geometry_count() const { return geometries_.size(); }

    // ========== Mechanics 组件管理（支持多套） ==========
    std::size_t add_mechanics(const Mechanics& mech);
    void remove_mechanics(std::size_t index);
    void set_active_mechanics(std::size_t index);

    [[nodiscard]] Mechanics* get_mechanics(std::size_t index);
    [[nodiscard]] const Mechanics* get_mechanics(std::size_t index) const;
    [[nodiscard]] Mechanics* get_active_mechanics();
    [[nodiscard]] const Mechanics* get_active_mechanics() const;
    [[nodiscard]] std::size_t mechanics_count() const { return mechanics_.size(); }
    [[nodiscard]] std::size_t get_active_mechanics_index() const { return active_mechanics_; }

    // ========== Optics 组件管理（支持多套） ==========
    std::size_t add_optics(const Optics& opt);
    void remove_optics(std::size_t index);
    void set_active_optics(std::size_t index);

    [[nodiscard]] Optics* get_optics(std::size_t index);
    [[nodiscard]] const Optics* get_optics(std::size_t index) const;
    [[nodiscard]] Optics* get_active_optics();
    [[nodiscard]] const Optics* get_active_optics() const;
    [[nodiscard]] std::size_t optics_count() const { return optics_.size(); }
    [[nodiscard]] std::size_t get_active_optics_index() const { return active_optics_; }

    // ========== Kinematics 组件管理（支持多套） ==========
    std::size_t add_kinematics(const Kinematics& kin);
    void remove_kinematics(std::size_t index);
    void set_active_kinematics(std::size_t index);

    [[nodiscard]] Kinematics* get_kinematics(std::size_t index);
    [[nodiscard]] const Kinematics* get_kinematics(std::size_t index) const;
    [[nodiscard]] Kinematics* get_active_kinematics();
    [[nodiscard]] const Kinematics* get_active_kinematics() const;
    [[nodiscard]] std::size_t kinematics_count() const { return kinematics_.size(); }
    [[nodiscard]] std::size_t get_active_kinematics_index() const { return active_kinematics_; }

    // ========== Acoustics 组件管理（支持多套） ==========
    std::size_t add_acoustics(const Acoustics& aco);
    void remove_acoustics(std::size_t index);
    void set_active_acoustics(std::size_t index);

    [[nodiscard]] Acoustics* get_acoustics(std::size_t index);
    [[nodiscard]] const Acoustics* get_acoustics(std::size_t index) const;
    [[nodiscard]] Acoustics* get_active_acoustics();
    [[nodiscard]] const Acoustics* get_active_acoustics() const;
    [[nodiscard]] std::size_t acoustics_count() const { return acoustics_.size(); }
    [[nodiscard]] std::size_t get_active_acoustics_index() const { return active_acoustics_; }

    // ========== 配置集管理（快速切换） ==========
    struct ComponentProfile {
        std::size_t mechanics_index{0};
        std::size_t optics_index{0};
        std::size_t kinematics_index{0};
        std::size_t acoustics_index{0};
    };

    void activate_profile(const ComponentProfile& profile);
    [[nodiscard]] ComponentProfile get_current_profile() const;

   private:
    // Actor 拥有多个 Geometry
    std::vector<Geometry> geometries_;

    // Actor 拥有多套组件
    std::vector<Mechanics> mechanics_;
    std::vector<Optics> optics_;
    std::vector<Kinematics> kinematics_;
    std::vector<Acoustics> acoustics_;

    // 当前激活的组件索引
    std::size_t active_mechanics_{0};
    std::size_t active_optics_{0};
    std::size_t active_kinematics_{0};
    std::size_t active_acoustics_{0};
};

// ============================================================================
// Camera: 相机类
// ============================================================================
class Camera {
   public:
    Camera();
    Camera(const std::array<float, 3>& position, const std::array<float, 3>& forward,
           const std::array<float, 3>& world_up, float fov);
    virtual ~Camera();

    [[nodiscard]] std::uintptr_t get_handle() const { return handle_; }

    void set(const std::array<float, 3>& position, const std::array<float, 3>& forward,
             const std::array<float, 3>& world_up, float fov) const;
    void set_surface(uint64_t surface);

    [[nodiscard]] std::array<float, 3> get_position() const;
    [[nodiscard]] std::array<float, 3> get_forward() const;
    [[nodiscard]] std::array<float, 3> get_world_up() const;
    [[nodiscard]] float get_fov() const;
    [[nodiscard]] uint64_t get_surface() const { return surface_; }

   private:
    std::uintptr_t handle_{};

    // Camera data
    std::array<float, 3> position_{};
    std::array<float, 3> forward{};
    std::array<float, 3> world_up_{};
    float fov_{60.0f};
    uint64_t surface_{0};
};

// ============================================================================
// ImageEffects: 图像效果类
// ============================================================================
class ImageEffects {
   public:
    ImageEffects();
    virtual ~ImageEffects();

   private:
    std::uintptr_t handle_{};
};

// ============================================================================
// Viewport: 视口类（OOP 设计）
// ============================================================================
class Viewport {
   public:
    Viewport();
    explicit Viewport(int width, int height);
    virtual ~Viewport();

    // ========== Camera 管理 ==========
    void set_camera(Camera* camera);
    [[nodiscard]] Camera* get_camera() { return camera_; }
    [[nodiscard]] const Camera* get_camera() const { return camera_; }
    [[nodiscard]] bool has_camera() const { return camera_ != nullptr; }
    void remove_camera();

    // ========== ImageEffects 管理 ==========
    void set_image_effects(ImageEffects* effects);
    [[nodiscard]] ImageEffects* get_image_effects() { return image_effects_; }
    [[nodiscard]] const ImageEffects* get_image_effects() const { return image_effects_; }
    [[nodiscard]] bool has_image_effects() const { return image_effects_ != nullptr; }
    void remove_image_effects();

    // ========== 视口属性 ==========
    void set_size(int width, int height);
    void set_position(int x, int y);
    void set_viewport_rect(int x, int y, int width, int height);

    [[nodiscard]] int get_width() const { return width_; }
    [[nodiscard]] int get_height() const { return height_; }
    [[nodiscard]] int get_x() const { return x_; }
    [[nodiscard]] int get_y() const { return y_; }
    [[nodiscard]] float get_aspect_ratio() const;

    // ========== 渲染表面 ==========
    void set_surface(void* surface);
    [[nodiscard]] void* get_surface() const { return surface_; }
    [[nodiscard]] bool has_surface() const { return surface_ != nullptr; }

    // ========== 渲染控制 ==========
    void set_clear_color(const std::array<float, 4>& color);
    [[nodiscard]] std::array<float, 4> get_clear_color() const { return clear_color_; }

    void set_enabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool is_enabled() const { return enabled_; }

    // ========== 交互功能 ==========
    Actor* pick_actor_at_pixel(int x, int y) const;
    void save_screenshot(const std::string& path) const;

   private:
    std::uintptr_t handle_{};

    // 关联对象（不拥有所有权）
    Camera* camera_{nullptr};
    ImageEffects* image_effects_{nullptr};

    // 视口属性
    int width_{1960};
    int height_{1080};
    int x_{0};
    int y_{0};

    // 渲染属性
    void* surface_{nullptr};
    std::array<float, 4> clear_color_{0.0f, 0.0f, 0.0f, 1.0f};
    bool enabled_{true};
};

// ============================================================================
// Environment: 环境类
// ============================================================================
class Environment {
   public:
    Environment();
    virtual ~Environment();

    void set_sun_direction(const std::array<float, 3>& direction) const;
    void set_floor_grid(bool enabled) const;

   private:
    std::uintptr_t handle_{};
};

// ============================================================================
// Scene: 场景类（OOP 设计）
// ============================================================================
class Scene {
   public:
    Scene();
    explicit Scene(bool light_field = false);
    virtual ~Scene();

    void set_active(bool active) { active_ = active; }
    [[nodiscard]] bool is_active() const { return active_; }

    // ========== Environment 管理 ==========
    void set_environment(Environment* env);
    [[nodiscard]] Environment* get_environment() { return environment_; }
    [[nodiscard]] const Environment* get_environment() const { return environment_; }
    [[nodiscard]] bool has_environment() const { return environment_ != nullptr; }
    void remove_environment();

    // ========== Actor 管理 ==========
    void add_actor(Actor* actor);
    void remove_actor(Actor* actor);
    void remove_actor_at(std::size_t index);
    void clear_actors();

    [[nodiscard]] Actor* get_actor(std::size_t index);
    [[nodiscard]] const Actor* get_actor(std::size_t index) const;
    [[nodiscard]] std::size_t actor_count() const { return actors_.size(); }
    [[nodiscard]] bool has_actor(const Actor* actor) const;

    // ========== Viewport 管理 ==========
    void add_viewport(Viewport* viewport);
    void remove_viewport(Viewport* viewport);
    void remove_viewport_at(std::size_t index);
    void clear_viewports();

    [[nodiscard]] Viewport* get_viewport(std::size_t index);
    [[nodiscard]] const Viewport* get_viewport(std::size_t index) const;
    [[nodiscard]] std::size_t viewport_count() const { return viewports_.size(); }
    [[nodiscard]] bool has_viewport(const Viewport* viewport) const;

   private:
    std::uintptr_t handle_{};

    // 场景属性
    bool active_{true};

    // 关联对象（不拥有所有权）
    Environment* environment_{nullptr};
    std::vector<Actor*> actors_;
    std::vector<Viewport*> viewports_;
};

// ============================================================================
// Scene I/O utilities
// ============================================================================
void read_scene(const std::filesystem::path& scene_path);
void write_scene(const std::filesystem::path& scene_path);

}  // namespace API
}  // namespace Corona
