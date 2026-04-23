using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.UI;

public class DetroitScanner : MonoBehaviour
{
    [Header("=== 材质和特效 (保持原样) ===")]
    public Material scanMaterial;
    public Material highlightMaterial;
    public Volume postVolume;

    [Header("=== 扫描设置 ===")]
    public float scanSpeed = 30f;
    public float maxRadius = 100f;
    [Range(0, 1)] public float darkAmount = 0.3f;
    public Color highlightColor = Color.cyan;
    public float maxHighlightAlpha = 0.5f;
    public float distortionHold = -0.5f;
    public float distortionRelease = 0.3f;
    public float recoilSpeed = 5f;

    [Header("=== 交互UI设置 ===")]
    public GameObject infoPanel;
    public Text titleText;
    public Text descriptionText;
    public float interactDistance = 5.0f; // 感应距离
    public LayerMask interactLayer; // 必须是 Interactable 层

    // 内部变量
    private bool isHolding = false;
    private float currentRadius = 0f;
    private float currentDim = 1f; private float targetDim = 1f;
    private float currentAlpha = 0f; private float targetAlpha = 0f;
    private LensDistortion lensDistortion;
    private ChromaticAberration chromaticAberration;

    void Start()
    {
        if (scanMaterial != null) { scanMaterial.SetFloat("_ScanRadius", -1f); scanMaterial.SetFloat("_BackgroundDim", 1f); }
        if (highlightMaterial != null) { Color c = highlightColor; c.a = 0f; highlightMaterial.color = c; }
        if (postVolume != null) { postVolume.profile.TryGet(out lensDistortion); postVolume.profile.TryGet(out chromaticAberration); }
        if (infoPanel != null) infoPanel.SetActive(false);
    }

    void Update()
    {
        // 1. 输入处理
        if (Input.GetMouseButtonDown(0))
        {
            isHolding = true;
            Ray ray = Camera.main.ViewportPointToRay(new Vector3(0.5f, 0.5f, 0));
            if (Physics.Raycast(ray, out RaycastHit hit))
            {
                if (scanMaterial != null) scanMaterial.SetVector("_ScanOrigin", hit.point);
            }
            currentRadius = 0;
        }

        if (Input.GetMouseButtonUp(0))
        {
            isHolding = false;
            if (lensDistortion != null) lensDistortion.intensity.value = distortionRelease;
            if (infoPanel != null) infoPanel.SetActive(false);
        }

        // 2. 状态平滑过渡
        if (isHolding)
        {
            currentRadius += scanSpeed * Time.deltaTime;
            if (currentRadius > maxRadius) currentRadius = maxRadius;
            targetDim = darkAmount; targetAlpha = maxHighlightAlpha;
            if (lensDistortion != null) lensDistortion.intensity.value = Mathf.Lerp(lensDistortion.intensity.value, distortionHold, Time.deltaTime * 2f);
            if (chromaticAberration != null) chromaticAberration.intensity.value = Mathf.Lerp(chromaticAberration.intensity.value, 1f, Time.deltaTime * 2f);
        }
        else
        {
            currentRadius = Mathf.Lerp(currentRadius, maxRadius * 1.5f, Time.deltaTime * 5f);
            targetDim = 1f; targetAlpha = 0f;
            if (lensDistortion != null) lensDistortion.intensity.value = Mathf.Lerp(lensDistortion.intensity.value, 0f, Time.deltaTime * recoilSpeed);
            if (chromaticAberration != null) chromaticAberration.intensity.value = Mathf.Lerp(chromaticAberration.intensity.value, 0f, Time.deltaTime * recoilSpeed);
        }

        currentDim = Mathf.Lerp(currentDim, targetDim, Time.deltaTime * 5f);
        currentAlpha = Mathf.Lerp(currentAlpha, targetAlpha, Time.deltaTime * 5f);

        if (scanMaterial != null) { scanMaterial.SetFloat("_ScanRadius", currentRadius); scanMaterial.SetFloat("_BackgroundDim", currentDim); }
        if (highlightMaterial != null) { Color c = highlightColor; c.a = currentAlpha; highlightMaterial.color = c; }

        HandleFloatingUI();
    }

    void HandleFloatingUI()
    {
        if (infoPanel == null) return;
        bool showUI = false;

        if (isHolding)
        {
            Collider[] hitColliders = Physics.OverlapSphere(transform.position, interactDistance, interactLayer);
            ScannableItem closestItem = null;
            float minDistance = float.MaxValue;

            foreach (var hit in hitColliders)
            {
                ScannableItem item = hit.GetComponent<ScannableItem>();
                if (item != null)
                {
                    float dist = Vector3.Distance(transform.position, item.transform.position);
                    if (dist < minDistance) { minDistance = dist; closestItem = item; }
                }
            }

            if (closestItem != null && Input.GetKey(KeyCode.E))
            {
                // === 这里是收集逻辑 ===
                if (!closestItem.isCollected && Input.GetKeyDown(KeyCode.E))
                {
                    closestItem.isCollected = true;
                    // 告诉总管加分
                    if (GameController.Instance != null) GameController.Instance.AddClue();

                    closestItem.itemName = "[已录入] " + closestItem.itemName;
                }

                showUI = true;
                if (!infoPanel.activeSelf) infoPanel.SetActive(true);
                if (titleText != null) titleText.text = closestItem.itemName;
                if (descriptionText != null) descriptionText.text = closestItem.itemDescription;

                Vector3 screenPos = Camera.main.WorldToScreenPoint(closestItem.transform.position + closestItem.uiOffset);
                if (screenPos.z > 0) infoPanel.transform.position = screenPos;
                else showUI = false;
            }
        }
        if (!showUI && infoPanel.activeSelf) infoPanel.SetActive(false);
    }

    void OnDrawGizmosSelected() { Gizmos.color = Color.yellow; Gizmos.DrawWireSphere(transform.position, interactDistance); }
}